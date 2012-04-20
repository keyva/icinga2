#ifndef CONFIGRPCCOMPONENT_H
#define CONFIGRPCCOMPONENT_H

namespace icinga
{

class ConfigRpcComponent : public Component
{
private:
	VirtualEndpoint::Ptr m_ConfigRpcEndpoint;

	IcingaApplication::Ptr GetIcingaApplication(void);

	int NewEndpointHandler(const NewEndpointEventArgs& ea);
	int WelcomeMessageHandler(const NewRequestEventArgs& ea);

	int LocalObjectCreatedHandler(const EventArgs& ea);
	int LocalObjectRemovedHandler(const EventArgs& ea);
	int LocalPropertyChangedHandler(const DictionaryPropertyChangedEventArgs& ea);

	int FetchObjectsHandler(const NewRequestEventArgs& ea);
	int RemoteObjectUpdatedHandler(const NewRequestEventArgs& ea);
	int RemoteObjectRemovedHandler(const NewRequestEventArgs& ea);

	JsonRpcRequest MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties);

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* CONFIGRPCCOMPONENT_H */
