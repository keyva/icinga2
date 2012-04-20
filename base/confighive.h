#ifndef CONFIGHIVE_H
#define CONFIGHIVE_H

namespace icinga
{

class I2_BASE_API ConfigHive : public Object
{
public:
	typedef shared_ptr<ConfigHive> Ptr;
	typedef weak_ptr<ConfigHive> WeakPtr;

	typedef map<string, ConfigCollection::Ptr>::iterator CollectionIterator;
	map<string, ConfigCollection::Ptr> Collections;

	void AddObject(const ConfigObject::Ptr& object);
	void RemoveObject(const ConfigObject::Ptr& object);
	ConfigObject::Ptr GetObject(const string& collection, const string& name = string());
	ConfigCollection::Ptr GetCollection(const string& collection);

	void ForEachObject(const string& type, function<int (const EventArgs&)> callback);

	Event<EventArgs> OnObjectCreated;
	Event<EventArgs> OnObjectRemoved;
	Event<DictionaryPropertyChangedEventArgs> OnPropertyChanged;
};

}

#endif /* CONFIGHIVE_H */
