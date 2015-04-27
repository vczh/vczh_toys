/*
API:
    this.__Type                                 // Get the real type that creates this object.
    this.__ScopeType                            // Get the scope type that creates this object.
    this.__ExternalReference                    // Get the external reference of this instance, for passing the value of "this" out of the class.
    this.__Dynamic(type)                        // Get the dynamic scope object of a base class.
    this.__Static(type)                         // Get the static scope object of a base class.
    this.__InitBase(type, [arguments])          // Call the constructor of the base class.

    obj.__Type                                  // Get the real type that creates this object.
    obj.__Dynamic(type)                         // Get the dynamic scope object of a base class.
    obj.__Static(type)                          // Get the static scope object of a base class.
    scope.__Original                            // Get the original object that creates this scope object.

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
            readonly: true | false,             // (optional), false
            hasEvent: true | false,             // (optional), false
            getterName: "GetterNameToOverride", // (optional), "GetMember"
            setterName: "SetterNameToOverride", // (optional), "SetMember",     implies readonly: false
            eventName: "EventNameToOverride",   // (optional), "MemberChanged", implies hasEvent: true
        }),
    });

    class Type {
        bool                        VirtualClass            // True if this type contains unoverrided abstract members
        string                      FullName;               // Get the full name
        map<string, __MemberBase>   Description;            // Get all declared members in this type
        map<string, __MemberBase>   FlattenedDescription;   // Get all potentially visible members in this type
        __BaseClass[]               BaseClasses;            // Get all direct base classes of this type
        __BaseClass[]               FlattenedBaseClasses;   // Get all direct or indirect base classes of this type

        bool IsAssignableFrom(Type childType);              // Returns true if "childType" is or inherits from "Type"
    }

    class __BaseClass {
        Type                        Type;
        bool                        Virtual;
    }

    class __MemberBase {
        enum <VirtualType> {
            NORMAL,
            VIRTUAL,
            OVERRIDE,
        }

        Type            DeclaringType;
        <VirtualType>   Virtual;
        bool            New;
        object          Value;
        __MemberBase[]  HiddenMembers;
    }

    class __PrivateMember : __MemberBase {}
    class __ProtectedMember : __MemberBase {}
    class __PublicMember : __MemberBase {}
*/

function __BaseClass(value, virtual) {
    this.Type = value;
    this.Virtual = virtual;
    Object.seal(this);
}

function __MemberBase() {
    this.DeclaringType = null;
    this.Virtual = __MemberBase.NORMAL;
    this.New = false;
    this.Value = null;
    this.HiddenMembers = [];
    Object.seal(this);
}
Object.defineProperty(__MemberBase, "NORMAL", {
    configurable: false,
    enumerable: true,
    writable: false,
    value: 1,
});
Object.defineProperty(__MemberBase, "ABSTRACT", {
    configurable: false,
    enumerable: true,
    writable: false,
    value: 2,
});
Object.defineProperty(__MemberBase, "VIRTUAL", {
    configurable: false,
    enumerable: true,
    writable: false,
    value: 3,
});
Object.defineProperty(__MemberBase, "OVERRIDE", {
    configurable: false,
    enumerable: true,
    writable: false,
    value: 4,
});

function __PrivateMember(value) {
    __MemberBase.call(this);
    this.Value = value;
}
__PrivateMember.prototype.__proto__ = __MemberBase.prototype;

function __ProtectedMember(value) {
    __MemberBase.call(this);
    this.Value = value;
}
__ProtectedMember.prototype.__proto__ = __MemberBase.prototype;

function __PublicMember(value) {
    __MemberBase.call(this);
    this.Value = value;
}
__PublicMember.prototype.__proto__ = __MemberBase.prototype;

function __EventHandler(func) {
    this.Function = func;
    Object.seal(this);
}

function __Event() {
    var handlers = [];

    Object.defineProperty(this, "Attach", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: function (func) {
            if (typeof func !== "function") {
                throw new Error("Only functions can be attached to an event.");
            }
            var handler = new __EventHandler(func);
            handlers.push(handler);
            return handler;
        }
    });

    Object.defineProperty(this, "Detach", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: function (handler) {
            var index = handlers.indexOf(handler);
            if (index === -1) {
                throw new Error("Only handlers that created by this event can detach.");
            }
            handlers.splice(index, 1);
        }
    });

    Object.defineProperty(this, "Execute", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: function () {
            for (var i in handlers) {
                handlers[i].Function.apply(null, arguments);
            }
        }
    });

    Object.seal(this);
}

function __Property() {
    this.Readonly = false;
    this.HasEvent = false;
    this.GetterName = null;
    this.SetterName = null;
    this.EventName = null;
    Object.seal(this);
}

function __BuildOverloadingFunctions() {
    if (arguments.length % 2 !== 0) {
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
            if (arguments.length !== typeList.length) continue;

            var matched = typeList.length === 0;
            for (var j in typeList) {
                var arg = arguments[j];

                var type = typeList[j];
                if (type === Number) {
                    matched = typeof (arg) === "number";
                }
                else if (type === Boolean) {
                    matched = typeof (arg) === "boolean";
                }
                else if (type === String) {
                    matched = typeof (arg) === "string";
                }
                else if (type === Array) {
                    matched = arg instanceof Array;
                }
                else if (type === Function) {
                    matched = typeof (arg) === "function";
                }
                else if (type === Object) {
                    matched = typeof (arg) === "object";
                }
                else if (arg === undefined) {
                    matched = false;
                }
                else if (arg !== null) {
                    if (type.prototype.__proto__ === Class) {
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
                if (!matched) break;
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
            if (typeof member.Value !== "function") {
                if (member.Virtual !== __MemberBase.NORMAL) {
                    throw new Error("Only function member can be virtual or override.");
                }
            }
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
        member.Virtual = __MemberBase.ABSTRACT;
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
            member.Value.Readonly = value.hasOwnProperty("getterName") && !value.hasOwnProperty("setterName");
        }

        if (value.hasOwnProperty("hasEvent")) {
            member.Value.HasEvent = value.hasEvent;
            if (!value.hasEvent && value.hasOwnProperty("eventName")) {
                throw new Error("Non-trigger property cannot have an event.");
            }
        }
        else {
            member.Value.HasEvent = value.hasOwnProperty("eventName");
        }

        if (value.hasOwnProperty("getterName")) {
            member.Value.GetterName = value.getterName;
        }
        if (value.hasOwnProperty("setterName")) {
            member.Value.SetterName = value.setterName;
        }
        if (value.hasOwnProperty("eventName")) {
            member.Value.EventName = value.eventName;
        }
    });
}

function Virtual(value) {
    return new __BaseClass(value, true);
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

    function CreateInternalReference(typeObject) {
        // create an internal reference from a type description and copy all members
        var description = typeObject.Description;
        var internalReference = {};

        // this.__ScopeType
        internalReference.__ScopeType = typeObject;

        for (var name in description) {
            var member = description[name];

            if (member instanceof __PrivateMember ||
                member instanceof __ProtectedMember ||
                member instanceof __PublicMember) {
                if (member.Value instanceof __Property) {
                    (function () {
                        var getterName = member.Value.GetterName;
                        var setterName = member.Value.SetterName;

                        Object.defineProperty(internalReference, name, {
                            configurable: true,
                            enumerable: true,
                            get: function () {
                                return internalReference[getterName].apply(internalReference, []);
                            },
                            set: member.Value.Readonly ? undefined : function (value) {
                                internalReference[setterName].apply(internalReference, [value]);
                            },
                        });
                    })();
                }
                else {
                    Object.defineProperty(internalReference, name, {
                        configurable: true,
                        enumerable: true,
                        writable: typeof member.Value !== "function" && !(member.Value instanceof __Event),
                        value: member.Value,
                    });
                }
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

        if (typeof member.Value === "function") {
            Object.defineProperty(target, memberName, {
                configurable: true,
                enumerable: true,
                writable: false,
                value: function () {
                    return member.Value.apply(source, arguments);
                }
            });
        }
        else {
            var readonly = member.Value instanceof __Property && member.Value.Readonly;
            Object.defineProperty(target, memberName, {
                configurable: true,
                enumerable: true,
                get: function () {
                    return source[memberName];
                },
                set: readonly ? undefined : function (value) {
                    source[memberName] = value;
                }
            });
        }
    }

    function CopyReferencableMembers(target, source, description, forInternalReference) {
        // copy all closured members from one internal reference to another
        for (var name in description) {
            if (name !== "__Constructor") {
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

    function OverrideVirtualFunction(source, memberName, member, targetBaseClasses, accumulated) {
        // override a virtual functions in base internal references
        for (var i in targetBaseClasses) {
            var targetType = targetBaseClasses[i].Type;
            var target = accumulated[targetType.FullName];
            var targetDescription = targetType.FlattenedDescription;
            var targetMember = targetDescription[memberName];
            if (targetMember !== undefined) {
                if (targetMember.Virtual !== __MemberBase.NORMAL) {
                    CopyReferencableMember(target, source, memberName, member, true, true);
                }
                if (targetMember.New === true) {
                    continue;
                }
            }
            OverrideVirtualFunction(source, memberName, member, targetType.BaseClasses, accumulated);
        }
    }

    function OverrideVirtualFunctions(source, sourceType, accumulated) {
        // override every virtual functions in base internal references
        var description = sourceType.Description;
        for (var name in description) {
            var member = description[name];
            if (member.Virtual === __MemberBase.OVERRIDE) {
                OverrideVirtualFunction(source, name, member, sourceType.BaseClasses, accumulated);
            }
        }
    }

    function CreateCompleteInternalReference(type, accumulated, forVirtualBaseClass) {
        // create an internal reference from a type with inherited members
        var description = type.Description;
        var baseClasses = type.BaseClasses;
        var baseInstances = new Array(baseClasses.length);

        for (var i = 0; i <= baseClasses.length; i++) {
            if (i === baseClasses.length) {
                // only create one internal reference for one virtual base class
                var instanceReference = accumulated[type.FullName];
                if (instanceReference !== undefined) {
                    if (forVirtualBaseClass) {
                        return instanceReference;
                    }
                    else {
                        throw new Error("Internal error: Internal reference for type \"" + type.FullName + "\" has already been created.");
                    }
                }

                // create the internal reference for the current type
                internalReference = CreateInternalReference(type);

                // override virtual functions in base internal references
                OverrideVirtualFunctions(internalReference, type, accumulated);

                // inherit members from base classes
                for (var j = 0; j < baseClasses.length; j++) {
                    CopyReferencableMembers(
                        internalReference,
                        baseInstances[j],
                        baseClasses[j].Type.Description,
                        true);
                }

                // this.__FromVirtualBaseClass (deleted after constructor)
                internalReference.__FromVirtualBaseClass = forVirtualBaseClass;

                if (description.__Constructor !== undefined) {
                    // this.__ConstructedBy (deleted after constructor)
                    internalReference.__ConstructedBy = null;

                    // this.__CanBeConstructedBy (deleted after constructor)
                    internalReference.__CanBeConstructedBy = null;
                }

                accumulated[type.FullName] = internalReference;
                return internalReference;
            }
            else {
                // create a complete internal reference for a base class
                var baseClass = baseClasses[i];
                var baseInstance = CreateCompleteInternalReference(
                    baseClass.Type,
                    accumulated,
                    baseClass.Virtual);
                baseInstance.__CanBeConstructedBy = type;
                baseInstances[i] = baseInstance;
            }
        }
    }

    function InjectObjects(externalReference, typeObject, accumulated) {

        diScope = {};
        deScope = {};
        siScope = {};
        seScope = {};

        function GetScope(type, isDynamic, isInternal) {
            if (typeObject === type || !accumulated.hasOwnProperty(type.FullName)) {
                throw new Error("Type \"" + typeObject.FullName + "\" does not directly or indirectly inherit from \"" + type.FullName + "\".");
            }

            var scopeCache = (isDynamic ? (isInternal ? diScope : deScope) : (isInternal ? siScope : seScope));
            var scopeObject = scopeCache[type.FullName];

            if (scopeObject === undefined) {
                scopeObject = {};
                if (isDynamic) {
                    Object.defineProperty(scopeObject, "__Original", {
                        configurable: false,
                        enumerable: false,
                        writable: false,
                        value: accumulated[typeObject.FullName],
                    });
                }
                else {
                    Object.defineProperty(scopeObject, "__Original", {
                        configurable: false,
                        enumerable: false,
                        writable: false,
                        value: externalReference,
                    });
                }

                var flattened = type.FlattenedDescription;
                for (var i in flattened) {
                    (function () {
                        var memberName = i;
                        var member = flattened[memberName];
                        if (member instanceof __PublicMember || (isInternal && member instanceof __ProtectedMember)) {
                            ref = accumulated[type.FullName];

                            if (isDynamic) {
                                var prop = Object.getOwnPropertyDescriptor(ref, memberName);

                                if (prop.get !== undefined) { // property
                                    Object.defineProperty(scopeObject, memberName, prop);
                                }
                                else if (prop.writable === false) {
                                    if (prop.value instanceof __Event) { // event
                                        Object.defineProperty(scopeObject, memberName, prop);
                                    }
                                    else { // function
                                        Object.defineProperty(scopeObject, memberName, {
                                            configurable: false,
                                            enumerable: true,
                                            writable: false,
                                            value: function () {
                                                return prop.value.apply(ref, arguments);
                                            }
                                        });
                                    }
                                }
                                else { // field
                                    Object.defineProperty(scopeObject, memberName, {
                                        configurable: false,
                                        enumerable: true,
                                        get: function () {
                                            return ref[memberName];
                                        },
                                        set: function (value) {
                                            ref[memberName] = value;
                                        },
                                    });
                                }
                            }
                            else {
                                CopyReferencableMember(
                                    scopeObject,
                                    ref,
                                    memberName,
                                    member,
                                    false);
                            }
                        }
                    })();
                }

                Object.seal(scopeObject);
                scopeCache[type.FullName] = scopeObject;
            }

            return scopeObject;
        }

        // externalReference.__Type
        Object.defineProperty(externalReference, "__Type", {
            configurable: false,
            enumerable: false,
            writable: false,
            value: typeObject,
        });

        // externalReference.__Dynamic
        Object.defineProperty(externalReference, "__Dynamic", {
            configurable: false,
            enumerable: false,
            writable: false,
            value: function (type) {
                return GetScope(type, true, false);
            },
        });

        // externalReference.__Static
        Object.defineProperty(externalReference, "__Static", {
            configurable: false,
            enumerable: false,
            writable: false,
            value: function (type) {
                return GetScope(type, false, false);
            },
        });

        for (var i in accumulated) {
            (function () {
                var ref = accumulated[i];
                var refType = ref.__ScopeType;

                // this.__Type
                Object.defineProperty(ref, "__Type", {
                    configurable: false,
                    enumerable: false,
                    writable: false,
                    value: typeObject,
                });

                // this.__ExternalReference
                Object.defineProperty(ref, "__ExternalReference", {
                    configurable: false,
                    enumerable: false,
                    writable: false,
                    value: externalReference,
                });

                // this.__Dynamic(type)
                Object.defineProperty(ref, "__Dynamic", {
                    configurable: false,
                    enumerable: false,
                    writable: false,
                    value: function (type) {
                        return GetScope(type, true, true);
                    },
                });

                // this.__Static(type)
                Object.defineProperty(ref, "__Static", {
                    configurable: false,
                    enumerable: false,
                    writable: false,
                    value: function (type) {
                        return GetScope(type, false, true);
                    },
                });

                // this.__InitBase(type) (deleted after constructor)
                Object.defineProperty(ref, "__InitBase", {
                    configurable: true,
                    enumerable: false,
                    writable: false,
                    value: function (type, args) {
                        var baseRef = accumulated[type.FullName];
                        if (baseRef === undefined) {
                            throw new Error("Type \"" + refType.FullName + "\" does not directly inherit from \"" + type.FullName + "\".");
                        }

                        if (baseRef.__CanBeConstructedBy !== refType) {
                            if (baseRef.__FromVirtualBaseClass) {
                                return;
                            }
                            else {
                                throw new Error("In the construction of type \"" + typeObject.FullName + "\", type \"" + refType.FullName + "\" cannot initialize type \"" + type.FullName + "\" because the correct type to initialize it is \"" + baseRef.__CanBeConstructedBy.FullName + "\".");
                            }
                        }

                        var ctor = baseRef.__Constructor;
                        if (ctor === undefined) {
                            throw new Error("Type\"" + type.FullName + "\" does not have a constructor.");
                        }

                        if (baseRef.__ConstructedBy === null) {
                            ctor.apply(baseRef, args);
                            baseRef.__ConstructedBy = refType;
                        }
                        else {
                            throw new Error("The constructor of type \"" + type.FullName + "\" has already been executed by \"" + baseRef.__ConstructedBy.FullName + "\".");
                        }
                    },
                });
            })();
        }
    }

    var directBaseClasses = new Array(arguments.length - 2);
    for (var i = 1; i < arguments.length - 1; i++) {
        baseClass = arguments[i];
        if (!(baseClass instanceof __BaseClass)) {
            baseClass = new __BaseClass(baseClass, false);
        }
        directBaseClasses[i - 1] = baseClass;
    }
    var description = arguments[arguments.length - 1];

    function Type() {
        var typeObject = arguments.callee;

        if (typeObject.VirtualClass) {
            for (var i in typeObject.FlattenedDescription) {
                var member = typeObject.FlattenedDescription[i];
                if (member.Virtual === __MemberBase.ABSTRACT) {
                    throw new Error("Cannot create instance of type \"" + typeObject.FullName + "\" because it contains an abstract function \"" + i + "\".");
                }
            }
        }

        // create every internalReference, which is the value of "this" in member functions
        var accumulated = {};
        var internalReference = CreateCompleteInternalReference(
            typeObject,
            accumulated,
            false);

        // create the externalReference
        var externalReference = {};

        // copy all public member fields to externalReference
        for (var i in accumulated) {
            var ref = accumulated[i];
            CopyReferencableMembers(
                externalReference,
                ref,
                ref.__ScopeType.Description,
                false);
        }

        // inject API into references
        InjectObjects(externalReference, typeObject, accumulated);

        // call the constructor
        externalReference.__proto__ = typeObject.prototype;
        if (internalReference.hasOwnProperty("__Constructor")) {
            internalReference.__Constructor.apply(internalReference, arguments);
        }

        // check is there any constructor is not called
        for (var i in accumulated) {
            var ref = accumulated[i];
            if (ref !== internalReference) {
                if (ref.__ConstructedBy === null) {
                    throw new Error("The constructor of type \"" + ref.__ScopeType.FullName + "\" has never been executed.");
                }
            }
        }

        // delete every __InitBase so that this function can only be called when constructing the object
        for (var i in accumulated) {
            delete accumulated[i].__FromVirtualBaseClass;
            delete accumulated[i].__ConstructedBy;
            delete accumulated[i].__CanBeConstructedBy;
            delete accumulated[i].__InitBase;
        }

        // return the created object
        for (var i in accumulated) {
            Object.seal(accumulated[i]);
        }
        Object.seal(externalReference);
        return externalReference;
    }

    // set __MemberBase.DeclaringType
    for (var name in description) {
        if (name.substring(0, 2) === "__" && name !== "__Constructor") {
            throw new Error("Member name cannot start with \"__\" except \"__Constructor\".");
        }

        var member = description[name];
        member.DeclaringType = Type;

        var value = member.Value;
        if (value instanceof __Property) {
            if (value.GetterName === null) {
                value.GetterName = "Get" + name;
            }
            if (!value.Readonly && value.SetterName === null) {
                value.SetterName = "Set" + name;
            }
            if (value.HasEvent && value.EventName === null) {
                value.EventName = name + "Changed";
            }
        }
    }

    // calculate Type.FlattenedBaseClasses
    var flattenedBaseClasses = [];
    var flattenedBaseClassNames = {};
    function AddFlattenedBaseClass(baseClass) {
        var existingBaseClass = flattenedBaseClassNames[baseClass.Type.FullName];
        if (existingBaseClass === undefined) {
            flattenedBaseClassNames[baseClass.Type.FullName] = baseClass;
            flattenedBaseClasses.push(baseClass);
        }
        else {
            if (existingBaseClass.Virtual !== true || baseClass.Virtual !== true) {
                throw new Error("Type \"" + fullName + "\" cannot non-virtually inherit from type \"" + baseClass.Type.FullName + "\" multiple times.");
            }
        }
    }

    for (var i in directBaseClasses) {
        var baseClass = directBaseClasses[i];
        var baseFlattened = baseClass.Type.FlattenedBaseClasses;
        for (var j in baseFlattened) {
            AddFlattenedBaseClass(baseFlattened[j]);
        }
        AddFlattenedBaseClass(baseClass);
    }

    // calculate Type.FlattenedDescription
    var flattenedDescription = Object.create(description);

    for (var i in directBaseClasses) {
        var baseClass = directBaseClasses[i];
        var flattened = baseClass.Type.FlattenedDescription;
        for (var name in flattened) {
            var member = description[name];
            var baseMember = flattened[name];

            if (name === "__Constructor") {
                continue;
            }

            if (baseMember instanceof __PrivateMember) {
                continue;
            }

            if (member === undefined) {
                if (flattenedDescription[name] !== undefined) {
                    if (flattenedBaseClassNames[baseMember.DeclaringType.FullName].Virtual === false) {
                        throw new Error("Type \"" + fullName + "\" cannot inherit multiple members of the same name \"" + name + "\" without defining a new one.");
                    }
                }
                else {
                    flattenedDescription[name] = baseMember;
                }
            }
            else {
                member.HiddenMembers.push(baseMember);
            }
        }
    }

    for (var i in description) {
        var member = description[i];

        for (var j in member.HiddenMembers) {
            var hiddenMember = member.HiddenMembers[j];
            if (hiddenMember.Value instanceof __Event) {
                throw new Error("Type \"" + fullName + "\" cannot hide event \"" + i + "\".");
            }
        }

        if (member.Virtual === __MemberBase.OVERRIDE) {
            if (member.HiddenMembers.length === 0) {
                throw new Error("Type \"" + fullName + "\" cannot find virtual function \"" + i + "\" to override.");
            }
            else {
                for (var j in member.HiddenMembers) {
                    var hiddenMember = member.HiddenMembers[j];
                    if (hiddenMember.Virtual === __MemberBase.NORMAL) {
                        throw new Error("Type \"" + fullName + "\" cannot override non-virtual function \"" + i + "\".");
                    }
                }
            }
        }
        else if (member.New) {
            if (member.HiddenMembers.length === 0) {
                throw new Error("Type \"" + fullName + "\" cannot define a new member \"" + i + "\" without hiding anything.");
            }
        }
        else {
            for (var j in member.HiddenMembers) {
                var hiddenMember = member.HiddenMembers[j];
                if (hiddenMember.Virtual === __MemberBase.NORMAL) {
                    throw new Error("Type \"" + fullName + "\" cannot hide member \"" + i + "\" without new.");
                }
                else {
                    throw new Error("Type \"" + fullName + "\" cannot hide virtual function \"" + i + "\" without overriding.");
                }
            }
        }
    }

    // Type.VirtualClass
    var isVirtualClass = false;
    for (var i in flattenedDescription) {
        var member = flattenedDescription[i];
        if (member.Virtual === __MemberBase.ABSTRACT) {
            isVirtualClass = true;
        }
    }
    Object.defineProperty(Type, "VirtualClass", {
        configurable: false,
        enumerable: true,
        writable: false,
        value: isVirtualClass,
    });

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
            if (childType === Type) {
                return true;
            }
            else {
                var baseClasses = childType.BaseClasses;
                for (var i in baseClasses) {
                    if (Type.IsAssignableFrom(baseClasses[i].Type)) {
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