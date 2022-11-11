/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "config/configitembuilder.hpp"
#include "config/applyrule.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"
#include "base/exception.hpp"

using namespace icinga;

INITIALIZE_ONCE([]() {
	ApplyRule::RegisterType("Service", { "Host" });
});

bool Service::EvaluateApplyRuleInstance(const Host::Ptr& host, const String& name, ScriptFrame& frame, const ApplyRule& rule, bool skipFilter)
{
	if (!skipFilter && !rule.EvaluateFilter(frame))
		return false;

	auto& di (rule.GetDebugInfo());

#ifdef _DEBUG
	Log(LogDebug, "Service")
		<< "Applying service '" << name << "' to host '" << host->GetName() << "' for rule " << di;
#endif /* _DEBUG */

	ConfigItemBuilder builder{di};
	builder.SetType(Service::TypeInstance);
	builder.SetName(name);
	builder.SetScope(frame.Locals->ShallowClone());
	builder.SetIgnoreOnError(rule.GetIgnoreOnError());

	builder.AddExpression(new ImportDefaultTemplatesExpression());

	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "name"), OpSetLiteral, MakeLiteral(name), di));

	String zone = host->GetZoneName();

	if (!zone.IsEmpty())
		builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "package"), OpSetLiteral, MakeLiteral(rule.GetPackage()), di));

	builder.AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr serviceItem = builder.Compile();
	serviceItem->Register();

	return true;
}

bool Service::EvaluateApplyRule(
	const Host::Ptr& host, const ApplyRule& rule,
	TotalTimeSpentOnApplyMismatches& totalTimeSpentOnApplyMismatches, bool skipFilter
)
{
	bool match = false;
	BenchmarkApplyRuleEvaluation bare (totalTimeSpentOnApplyMismatches, match);

	auto& di (rule.GetDebugInfo());

	CONTEXT("Evaluating 'apply' rule (" << di << ")");

	ScriptFrame frame (false);

	if (rule.GetScope() || rule.GetFTerm()) {
		frame.Locals = new Dictionary();

		if (rule.GetScope()) {
			rule.GetScope()->CopyTo(frame.Locals);
		}

		host->GetFrozenLocalsForApply()->CopyTo(frame.Locals);
		frame.Locals->Freeze();
	} else {
		frame.Locals = host->GetFrozenLocalsForApply();
	}

	if (rule.GetFTerm()) {
		Value vinstances;

		try {
			vinstances = rule.GetFTerm()->Evaluate(frame);
		} catch (const std::exception&) {
			/* Silently ignore errors here and assume there are no instances. */
			return false;
		}

		if (vinstances.IsObjectType<Array>()) {
			if (!rule.GetFVVar().IsEmpty())
				BOOST_THROW_EXCEPTION(ScriptError("Dictionary iterator requires value to be a dictionary.", di));

			Array::Ptr arr = vinstances;

			ObjectLock olock(arr);
			for (const Value& instance : arr) {
				String name = rule.GetName();

				if (!rule.GetFKVar().IsEmpty()) {
					frame.Locals->Set(rule.GetFKVar(), instance, true);
					name += instance;
				}

				if (EvaluateApplyRuleInstance(host, name, frame, rule, skipFilter))
					match = true;
			}
		} else if (vinstances.IsObjectType<Dictionary>()) {
			if (rule.GetFVVar().IsEmpty())
				BOOST_THROW_EXCEPTION(ScriptError("Array iterator requires value to be an array.", di));

			Dictionary::Ptr dict = vinstances;
			ObjectLock olock (dict);

			for (auto& kv : dict) {
				frame.Locals->Set(rule.GetFKVar(), kv.first, true);
				frame.Locals->Set(rule.GetFVVar(), kv.second, true);

				if (EvaluateApplyRuleInstance(host, rule.GetName() + kv.first, frame, rule, skipFilter))
					match = true;
			}
		}
	} else if (EvaluateApplyRuleInstance(host, rule.GetName(), frame, rule, skipFilter)) {
		match = true;
	}

	return match;
}

void Service::EvaluateApplyRules(const Host::Ptr& host, TotalTimeSpentOnApplyMismatches& totalTimeSpentOnApplyMismatches)
{
	CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

	for (auto& rule : ApplyRule::GetRules(Service::TypeInstance, Host::TypeInstance)) {
		if (EvaluateApplyRule(host, *rule, totalTimeSpentOnApplyMismatches))
			rule->AddMatch();
	}

	for (auto& rule : ApplyRule::GetTargetedHostRules(Service::TypeInstance, host->GetName())) {
		if (EvaluateApplyRule(host, *rule, totalTimeSpentOnApplyMismatches, true))
			rule->AddMatch();
	}
}
