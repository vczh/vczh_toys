/*
API:
    this.__GetType()                            // Get the real type that creates this object.
    this.__GetBase(type)                        // Get the internal reference of a base type.
    this.__GetExternalReference()               // Get the external reference of this instance, for passing the value of "this" out of the class.

    Type.Name                                   // Get the full name
    Type.Description                            // Get all declared members in this type
    Type.FlattenedDescription                   // Get all potentially visible members in this type
    Type.BaseClasses                            // Get all direct base classes of this type
    Type.FlattenedBaseClasses                   // Get all direct or indirect base classes of this type
    Type.IsAssignableFrom(childType)            // Returns true if "childType" is or inherits from "Type"

    handler = Event.Attach(xxx);
    Event.Detach(handler);
    Event.Execute(...);

    Class(fullName, type1, Virtual(type2), ... {
        Member: (Public|Protected|Private) (value | function),
        Member: (Public|Protected|Private).Overload(typeList1, function1, typeList2, function2, ...);
        Member: (Public|Protected).(New|Virtual|NewVirtual|Override) (function),
        Member: (Public|Protected).Abstract();
        Member: Public.Event();
        Member: Public.Property({
            readonly: true | false,             // (optional, false)
            hasEvent: true | false,             // (optional, false)
            getterName: "GetterNameToOverride", // (optional, "GetMember")
            setterName: "SetterNameToOverride", // (optional, "SetMember")      implies readonly: false
            eventName: "EventNameToOverride",   // (optional, "MemberChanged")  implies hasEvent: true
        }),
    });
*/

function __MemberBase() {
}
Object.defineProperty(__MemberBase, "NORMAL", {
    configurable: false,
    enumerable: true,
    writable: false,
    value: 1,
});
Object.defineProperty(__MemberBase, "VIRTUAL", {
    configurable: false,
    enumerable: true,
    writable: false,
    value: 2,
});
Object.defineProperty(__MemberBase, "OVERRIDE", {
    configurable: false,
    enumerable: true,
    writable: false,
    value: 3,
});
Object.defineProperty(__MemberBase.prototype, "DeclaringType", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: null,
});
Object.defineProperty(__MemberBase.prototype, "Virtual", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: __MemberBase.NORMAL,
});
Object.defineProperty(__MemberBase.prototype, "New", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: false,
});
Object.defineProperty(__MemberBase.prototype, "Value", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: null,
});

function __PrivateMember(value) {
    this.Value = value;
}
__PrivateMember.prototype.__proto__ = __MemberBase.prototype;

function __ProtectedMember(value) {
    this.Value = value;
}
__ProtectedMember.prototype.__proto__ = __MemberBase.prototype;

function __PublicMember(value) {
    this.Value = value;
}
__PublicMember.prototype.__proto__ = __MemberBase.prototype;

function __EventHandler(func) {
    this.Function = func;
}
Object.defineProperty(__EventHandler.prototype, "Function", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: null,
});

function __Event() {
    var handlers = [];

    Object.defineProperty(this, "Attach", {
        configurable: false,
        enumerable: false,
        writable: false,
        value: function (func) {
            if (typeof func != "function") {
                throw new Error("Only functions can be attached to an event.");
            }
            var handler = new __EventHandler(func);
            handlers.push(handler);
            return handler;
        }
    });

    Object.defineProperty(this, "Detach", {
        configurable: false,
        enumerable: false,
        writable: false,
        value: function (handler) {
            var index = handlers.indexOf(handler);
            if (index == -1) {
                throw new Error("Only handlers that created by this event can detach.");
            }
            handlers.splice(index, 1);
        }
    });

    Object.defineProperty(this, "Execute", {
        configurable: false,
        enumerable: false,
        writable: false,
        value: function () {
            for (var i in handlers) {
                handlers[i].Function.apply(null, arguments);
            }
        }
    });
}

function __Property(func) {
    this.Function = func;
}
Object.defineProperty(__Property.prototype, "Readonly", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: false,
});
Object.defineProperty(__Property.prototype, "HasEvent", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: false,
});
Object.defineProperty(__Property.prototype, "GetterName", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: null,
});
Object.defineProperty(__Property.prototype, "SetterName", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: null,
});
Object.defineProperty(__Property.prototype, "EventName", {
    configurable: false,
    enumerable: true,
    writable: true,
    value: null,
});

function __BuildOverloadingFunctions() {
    if (arguments.length % 2 != 0) {
        throw new Error("Arguments for Overload should be typeList1, func1, typeList2, func2, ...");
    }

    var functionCount = Math.floor(arguments.length / 2);
    var typeLists = new Array(functionCount);
    var funcs = new Array(functionCount);
    for (var i = 0; i < functionCount; i++) {
        typeLists[i] = arguments[i * 2];
        funcs[i] = arguments[i * 2 + 1];
    }

    return function () {
        for (var i in typeLists) {
            var typeList = typeLists[i];
            if (arguments.length != typeList.length) continue;

            var matched = true;
            for (var j in typeList) {
                if (!matched) break;
                var arg = arguments[j];

                var type = typeList[j];
                if (type == Number) {
                    matched = typeof (arg) == "number";
                }
                else if (type == Boolean) {
                    matched = typeof (arg) == "boolean";
                }
                else if (type == String) {
                    matched = typeof (arg) == "string";
                }
                else if (type == Array) {
                    matched = arg instanceof Array;
                }
                else if (type == Function) {
                    matched = typeof (arg) == "function";
                }
                else if (type == Object) {
                    matched = typeof (arg) == "object";
                }
                else if (arg == undefined) {
                    matched = false;
                }
                else if (arg != null) {
                    if (type.prototype.__proto__ == Class) {
                        if (arg instanceof Class) {
                            matched = type.IsAssignableFrom(arg.GetType());
                        }
                        else {
                            matched = false;
                        }
                    }
                    else {
                        matched = arg instanceof type;
                    }
                }
            }

            if (matched) {
                return funcs[i].apply(this, arguments);
            }
        }
        throw new Error("Cannot find a overloading function that matches the arguments.");
    }
}

function __DefineDecorator(accessor, name, decorator) {
    Object.defineProperty(accessor, name, {
        configurable: false,
        enumerable: true,
        writable: false,
        value: function (value) {
            var member = accessor(value);
            decorator(member, value);
            return member;
        }
    });
}

function __DefineSubDecorator(accessor, name, decorator) {
    Object.defineProperty(accessor, name, {
        configurable: false,
        enumerable: true,
        writable: false,
        value: function () {
            return accessor(decorator.apply(null, arguments));
        }
    });
}

function __DefineOverload(accessor) {
    __DefineSubDecorator(accessor, "Overload", __BuildOverloadingFunctions);
}

function __DefineNew(accessor) {
    __DefineDecorator(accessor, "New", function (member) {
        member.New = true;
    });
}

function __DefineVirtual(accessor) {
    __DefineDecorator(accessor, "Virtual", function (member) {
        member.Virtual = __MemberBase.VIRTUAL;
    });
}

function __DefineNewVirtual(accessor) {
    __DefineDecorator(accessor, "NewVirtual", function (member) {
        member.New = true;
        member.Virtual = __MemberBase.VIRTUAL;
    });
}

function __DefineOverride(accessor) {
    __DefineDecorator(accessor, "Override", function (member) {
        member.Virtual = __MemberBase.OVERRIDE;
    });
}

function __DefineAbstract(accessor) {
    __DefineDecorator(accessor, "Abstract", function (member) {
        member.Virtual = __MemberBase.VIRTUAL;
        member.Value = function () {
            throw new Error("Cannot call an abstract function.");
        }
    });
}

function __DefineEvent(accessor) {
    __DefineDecorator(accessor, "Event", function (member) {
        member.Value = new __Event();
    });
}

function __DefineProperty(accessor) {
    __DefineDecorator(accessor, "Property", function (member, value) {
        member.Virtual = __MemberBase.VIRTUAL;
        member.Value = new __Property();

        if (!value.hasOwnProperty("getterName") && value.hasOwnProperty("setterName")) {
            throw new Error("Getter of the property should be set if setter ia set.");
        }

        if (value.hasOwnProperty("readonly")) {
            member.Value.Readonly = value.readonly;
            if (!value.readonly && value.hasOwnProperty("setterName")) {
                throw new Error("Readonly event cannot have a setter.");
            }
        }
        else {
            member.value.Readonly = value.hasOwnProperty("getterName") && !value.hasOwnProperty("setterName");
        }

        if (value.hasOwnProperty("hasEvent")) {
            member.value.HasEvent = value.hasEvent;
            if (!value.hasEvent && value.hasOwnProperty("eventName")) {
                throw new Error("Non-trigger property cannot have an event.");
            }
        }
        else {
            member.value.HasEvent = value.hasOwnProperty("eventName");
        }

        if (value.hasOwnProperty("getterName")) {
            member.value.GetterName = value.getterName;
        }
        if (value.hasOwnProperty("setterName")) {
            member.value.SetterName = value.setterName;
        }
        if (value.hasOwnProperty("eventName")) {
            member.value.EventName = value.eventName;
        }
    });
}

function Private(value) {
    return new __PrivateMember(value);
}
__DefineOverload(Private);

function Protected(value) {
    return new __ProtectedMember(value);
}
__DefineOverload(Protected);
__DefineNew(Protected);
__DefineVirtual(Protected);
__DefineNewVirtual(Protected);
__DefineAbstract(Protected);
__DefineOverride(Protected);

function Public(value) {
    return new __PublicMember(value);
}
__DefineOverload(Public);
__DefineNew(Public);
__DefineVirtual(Public);
__DefineNewVirtual(Public);
__DefineAbstract(Public);
__DefineOverride(Public);
__DefineEvent(Public);
__DefineProperty(Public);

function Class(fullName) {

    function CreateInternalReference(description) {
        // create an internal reference from a type description and copy all members
        var internalReference = {};

        for (var name in description) {
            var member = description[name];

            if (member instanceof __PrivateMember ||
                member instanceof __ProtectedMember ||
                member instanceof __PublicMember) {
                Object.defineProperty(internalReference, name, {
                    configurable: member.Virtual != __MemberBase.NORMAL,
                    enumerable: true,
                    writable: typeof member.Value != "function" && !(member.Value instanceof __Event),
                    value: member.Value,
                });
            }
        }

        return internalReference;
    }

    function CopyReferencableMember(target, source, memberName, member, forceReplace) {
        // copy a closured member from one internal reference to another
        if (target.hasOwnProperty(memberName)) {
            if (!forceReplace) {
                return;
            }
        }

        if (typeof member.Value == "function") {
            Object.defineProperty(target, memberName, {
                configurable: member.Virtual != __MemberBase.NORMAL,
                enumerable: true,
                writable: false,
                value: function () {
                    return member.Value.apply(source, arguments);
                }
            });
        }
        else {
            Object.defineProperty(target, memberName, {
                configurable: false,
                enumerable: true,
                get: function () {
                    return source[memberName];
                },
                set: function (value) {
                    source[memberName] = value;
                }
            });
        }
    }

    function CopyReferencableMembers(target, source, description, forInternalReference) {
        // copy all closured members from one internal reference to another
        for (var name in description) {
            if (name != "__Constructor") {
                (function () {
                    var memberName = name;
                    var member = description[memberName];

                    if (member instanceof __ProtectedMember) {
                        if (forInternalReference) {
                            CopyReferencableMember(target, source, memberName, member, false);
                        }
                    }
                    else if (member instanceof __PublicMember) {
                        CopyReferencableMember(target, source, memberName, member, false);
                    }
                })();
            }
        }
    }

    function OverrideVirtualFunction(source, memberName, member, targetTypes, accumulatedBaseClasses, accumulatedBaseReferences) {
        // override a virtual functions in base internal references
        for (var i in targetTypes) {
            var targetType = targetTypes[i];
            var target = accumulatedBaseReferences[accumulatedBaseClasses.indexOf(targetType)];
            var targetDescription = targetType.Description;
            var targetMember = targetDescription[memberName];
            if (targetMember != undefined) {
                if (targetMember.Virtual == __MemberBase.VIRTUAL || targetMember.Virtual == __MemberBase.OVERRIDE) {
                    CopyReferencableMember(target, source, memberName, member, true);
                }
                if (targetMember.New == true) {
                    continue;
                }
            }
            OverrideVirtualFunction(source, memberName, member, targetType.BaseClasses, accumulatedBaseClasses, accumulatedBaseReferences);
        }
    }

    function OverrideVirtualFunctions(source, sourceType, accumulatedBaseClasses, accumulatedBaseReferences) {
        // override every virtual functions in base internal references
        var description = sourceType.Description;
        for (var name in description) {
            var member = description[name];
            if (member.Virtual == __MemberBase.OVERRIDE) {
                OverrideVirtualFunction(source, name, member, sourceType.BaseClasses, accumulatedBaseClasses, accumulatedBaseReferences);
            }
        }
    }

    function CreateCompleteInternalReference(type, accumulatedBaseClasses, accumulatedBaseReferences) {
        // create an internal reference from a type with inherited members
        var description = type.Description;
        var baseClasses = type.BaseClasses;

        for (var i = 0; i <= baseClasses.length; i++) {
            if (i == baseClasses.length) {
                // create the internal reference for the current type
                var internalReference = CreateInternalReference(description);

                // override virtual functions in base internal references
                OverrideVirtualFunctions(internalReference, type, accumulatedBaseClasses, accumulatedBaseReferences);

                // inherit members from base classes
                for (var j = 0; j < accumulatedBaseClasses.length; j++) {
                    CopyReferencableMembers(
                        internalReference,
                        accumulatedBaseReferences[j],
                        accumulatedBaseClasses[j].Description,
                        true);
                }

                accumulatedBaseClasses.push(type);
                accumulatedBaseReferences.push(internalReference);

                // implement __GetBase
                internalReference.__GetBase = function (baseType) {
                    return accumulatedBaseReferences[accumulatedBaseClasses.indexOf(baseType)];
                }

                return internalReference;
            }
            else {
                // create a complete internal reference for a base class
                var baseInstance = CreateCompleteInternalReference(
                    baseClasses[i],
                    accumulatedBaseClasses,
                    accumulatedBaseReferences);
            }
        }
    }

    function InjectInternalReference(internalReference, typeObject, externalReference) {
        // implement __GetType and __GetExternalReference
        internalReference.__GetType = function () {
            return typeObject;
        }

        internalReference.__GetExternalReference = function () {
            return externalReference;
        }
    }

    var directBaseClasses = new Array(arguments.length - 2);
    for (var i = 1; i < arguments.length - 1; i++) {
        directBaseClasses[i - 1] = arguments[i];
    }
    var description = arguments[arguments.length - 1];

    function Type() {
        var typeObject = arguments.callee;

        // create every internalReference, which is the value of "this" in member functions
        var accumulatedBaseClasses = [];
        var accumulatedBaseReferences = [];
        var internalReference = CreateCompleteInternalReference(
            typeObject,
            accumulatedBaseClasses,
            accumulatedBaseReferences);

        // create the externalReference
        var externalReference = {};

        // inject externalReference.GetType()
        Object.defineProperty(externalReference, "__GetType", {
            configurable: false,
            enumerable: true,
            writable: false,
            value: function () {
                return typeObject;
            }
        });

        // inject internalReference.__GetType()
        // inject internalReference.__GetExternalReference()
        for (var i in accumulatedBaseReferences) {
            InjectInternalReference(accumulatedBaseReferences[i], typeObject, externalReference);
        }

        // copy all public member fields to externalReference
        for (var i in accumulatedBaseReferences) {
            CopyReferencableMembers(
                externalReference,
                accumulatedBaseReferences[i],
                accumulatedBaseClasses[i].Description,
                false);
        }

        // return the externalReference
        externalReference.__proto__ = typeObject.prototype;
        if (internalReference.hasOwnProperty("__Constructor")) {
            internalReference.__Constructor.apply(internalReference, arguments);
        }
        return externalReference;
    }

    // set __MemberBase.DeclaringType
    for (var name in description) {
        description[name].DeclaringType = Type;
    }

    // calculate Type.FlattenedBaseClasses
    var flattenedBaseClasses = [];
    function AddFlattenedBaseClass(type) {
        if (flattenedBaseClasses.indexOf(type) == -1) {
            flattenedBaseClasses.push(type);
        }
        else {
            throw new Error("Cannot non-virtually inherit from type \"" + type.FullName + "\" multiple times.");
        }
    }

    for (var i in directBaseClasses) {
        var baseClass = directBaseClasses[i];
        var baseFlattened = baseClass.FlattenedBaseClasses;
        for (var j in baseFlattened) {
            AddFlattenedBaseClass(baseFlattened[j]);
        }
        AddFlattenedBaseClass(baseClass);
    }

    // calculate Type.FlattenedDescription
    var flattenedDescription = {};
    function AddFlattenedMember(name, member) {

        if (member.Virtual == __MemberBase.OVERRIDE) {
            return;
        }

        if (member.DeclaringType != Type) {
            if (name == "__Constructor") {
                return;
            }

            if (member instanceof __PrivateMember) {
                return;
            }

            if (flattenedDescription.hasOwnProperty(name)) {
                if (!description.hasOwnProperty(name)) {
                    throw new Error("Cannot inherit multiple members of the same name \"" + name + "\" without defining a new one.");
                }
            }
        }

        flattenedDescription[name] = member;
    }

    for (var i in directBaseClasses) {
        var baseClass = directBaseClasses[i];
        var flattened = baseClass.FlattenedDescription;
        for (var j in flattened) {
            AddFlattenedMember(j, flattened[j]);
        }
    }

    for (var i in description) {
        var member = description[i];
        var flattenedMember = flattenedDescription[i];

        if (flattenedMember == undefined) {
            if (member.Virtual == __MemberBase.OVERRIDE) {
                throw new Error("Cannot find virtual function \"" + i + "\" to override.");
            }
        }
        else {
            if (flattenedMember.Value instanceof __Event) {
                throw new Error("Cannot hide event \"" + i + "\".");
            }
            else if (member.Virtual == __MemberBase.OVERRIDE) {
                if (flattenedMember.Virtual == __MemberBase.NORMAL) {
                    throw new Error("Cannot override non-virtual function \"" + i + "\".");
                }
            }
            else if (!member.New) {
                if (flattenedMember.Virtual != __MemberBase.NORMAL) {
                    throw new Error("Cannot hide virtual function \"" + i + "\" without overriding.");
                }
            }
        }
    }

    for (var j in description) {
        AddFlattenedMember(j, description[j]);
    }

    // Type.FullName
    Object.defineProperty(Type, "FullName", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: fullName,
    });

    // Type.Description
    Object.defineProperty(Type, "Description", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: description,
    });

    // Type.FlattenedDescription
    Object.defineProperty(Type, "FlattenedDescription", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: flattenedDescription,
    });

    // Type.BaseClasses
    Object.defineProperty(Type, "BaseClasses", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: directBaseClasses,
    });

    // Type.FlattenedBaseClasses
    Object.defineProperty(Type, "FlattenedBaseClasses", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: flattenedBaseClasses,
    });

    // Type.IsAssignableFrom(childType)
    Object.defineProperty(Type, "IsAssignableFrom", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: function (childType) {
            if (childType == Type) {
                return true;
            }
            else {
                var baseClasses = childType.BaseClasses;
                for (var i in baseClasses) {
                    if (Type.IsAssignableFrom(baseClasses[i])) {
                        return true;
                    }
                }
                return false;
            }
        }
    });

    Type.prototype.__proto__ = Class.prototype;
    return Type;
}