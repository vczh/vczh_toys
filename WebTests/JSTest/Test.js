/*
API:
    assert(expression);
    emptyCase();

    testCase(name, function() {
        // test case
    });

    summaryTest();
*/

function assert(code) {
    if (!code) throw new Error("Fuck");
}

var __passedCases = 0;
var __failedCases = 0;
var __emptyCases = 0;

function testCase(name, code) {
    console.debug("Running: " + name);

    var div = document.createElement("div");
    div.appendChild(document.createTextNode(name));
    document.body.appendChild(div);

    try {
        code();
        div.setAttribute("style", "color: green;");
        __passedCases++;
    }
    catch (ex) {
        if (ex.message == "<EMPTY CASE>") {
            div.setAttribute("style", "color: purple;");
            __emptyCases++;
        }
        else {
            div.setAttribute("style", "color: red;");
            __failedCases++;
            code();
        }
    }
}

function emptyCase() {
    throw new Error("<EMPTY CASE>");
}

function summaryTest() {
    var container = document.createElement("div");
    container.setAttribute("style", "margin-top: 1.5em;");
    {
        var div = document.createElement("div");
        div.appendChild(document.createTextNode("=================================================="));
        div.setAttribute("style", "color: gray;");
        container.appendChild(div);
    }
    {
        var div = document.createElement("div");
        div.appendChild(document.createTextNode("Passed: " + __passedCases));
        div.setAttribute("style", "color: green; float: left");
        container.appendChild(div);
    }
    {
        var div = document.createElement("div");
        div.appendChild(document.createTextNode("\u00A0"));
        div.setAttribute("style", "min-width: 4em; float: left");
        container.appendChild(div);
    }
    {
        var div = document.createElement("div");
        div.appendChild(document.createTextNode("Failed: " + __failedCases));
        div.setAttribute("style", "color: red; float: left");
        container.appendChild(div);
    }
    {
        var div = document.createElement("div");
        div.appendChild(document.createTextNode("\u00A0"));
        div.setAttribute("style", "min-width: 4em; float: left");
        container.appendChild(div);
    }
    {
        var div = document.createElement("div");
        div.appendChild(document.createTextNode("Empty: " + __emptyCases));
        div.setAttribute("style", "color: purple; float: left");
        container.appendChild(div);
    }
    {
        var div = document.createElement("div");
        div.setAttribute("style", "claer: both");
        container.appendChild(div);
    }
    document.body.appendChild(container);
}