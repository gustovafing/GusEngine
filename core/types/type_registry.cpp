#include "type_registry.h"

#include "core/types/resource.h"
#include "project/resources/shader.h"
#include "project/resources/image.h"

using namespace engine_type_registry;
map<string, EngineClass> type_registry::_registered_classes{};
string type_registry::_currentClass = "";
void type_registry::register_all_types()
{
	using namespace resources;
	register_class<Resource>();
	register_class<Shader>();
	register_class<Image>();
}

void type_registry::register_new_class(string new_class_name, string parent_class_name)
{
	if (_registered_classes.contains(new_class_name)) {
		Log.Error("TypeRegistry", "Attempted to register class with name '" + new_class_name + "' twice.");
		return;
	}
	EngineClass newClass{};
	newClass._className = new_class_name;
	newClass._parentClassName = parent_class_name;
	if (newClass._parentClassName != "" && newClass._parentClassName != "Object") {
		newClass._inherits = &_registered_classes[newClass._parentClassName];
	}
	_registered_classes[new_class_name] = newClass;

	_currentClass = new_class_name;

}

void type_registry::class_define_property(ObjectRTTIModel::ObjectPropertyDefinition def) {
	if (_registered_classes[_currentClass]._properties.contains(def.propertyName)) {
		Log.Warn("TypeRegistry", "Attempted to redefine property " + _currentClass + "::" + def.propertyName);
		return;
	}
	_registered_classes[_currentClass]._properties[def.propertyName] = def;
}

