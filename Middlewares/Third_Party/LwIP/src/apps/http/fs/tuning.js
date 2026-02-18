function n(num, len) {
return `${num}`.padStart(len, '0');
}

function SendSettings() {
let tuning_json = JSON.parse(window.sessionStorage.getItem("tuning"));
let validate_status = 0;
const elem = document.getElementsByTagName("input");

let notify = document.getElementById("notice");
notify.style.color = "black";
notify.textContent = "";

for (let i = 0; i < elem.length; i++) {
const item = elem[i];

if (item.id == "hysteresis_window") {
let val = parseFloat(item.value);
if (val < 0.1 || val > 2.0) {
validate_status++;
notify.innerHTML += "Значение гистерезиса находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = Math.round(val * 100);
}
} else if (item.id == "self_test_duration") {
let val = parseInt(item.value);
if (val < 1 || val > 10) {
validate_status++;
notify.innerHTML += "Значение времени находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = val;
}
} else if (item.id == "min_stroke_rod") {
let val = parseInt(item.value);
if (val < 20 || val > 100) {
validate_status++;
notify.innerHTML += "Значение перемещения находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = val;
}
} else if (item.id == "min_stroke") {
let val = parseInt(item.value);
if (val < 0 || val > 100) {
validate_status++;
notify.innerHTML += "Значение положения находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = val;
}
} else if (item.id == "adc_min_stroke") {
let val = parseFloat(item.value);
if (val < 0 || val > 10) {
validate_status++;
notify.innerHTML += "Значение Напряжения находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = Math.round(val * 100);
}
} else if (item.id == "max_stroke") {
let val = parseInt(item.value);
if (val < 0 || val > 5000) {
validate_status++;
notify.innerHTML += "Значение положения за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = val;
}
} else if (item.id == "adc_max_stroke") {
let val = parseFloat(item.value);
if (val < 0 || val > 10) {
validate_status++;
notify.innerHTML += "Значение Напряжения находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = Math.round(val * 100);
}
} else if (item.id == "zero_offset") {
let val = parseInt(item.value);
if (val < -99 || val > 99) {
validate_status++;
notify.innerHTML += "Значение Напряжения находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = val;
}
} else if (item.id == "debounce_in") {
let val = parseInt(item.value);
if (val < 100 || val > 9999) {
validate_status++;
notify.innerHTML += "Значение времени находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = val;
}
} else if (item.id == "delay_out") {
let val = parseInt(item.value);
if (val < 1000 || val > 60000) {
validate_status++;
notify.innerHTML += "Значение времени находится за границами рабочего диапазона<br>";
} else {
tuning_json[item.id] = val;
}
}

/*
if(item.type == "text") {
console.log(i +"\t" +item.id+"\t" +item.name+"\t" +item.value);
}
*/
}

if (parseInt(tuning_json.min_stroke) >= parseInt(tuning_json.max_stroke)) {
notify.innerHTML += "min больше или равно max<br>";
validate_status++;
}
if (parseFloat(tuning_json.adc_min_stroke) >= parseFloat(tuning_json.adc_max_stroke)) {
notify.innerHTML += "min больше или равно max<br>";
validate_status++;
}
if (validate_status > 0) {
document.getElementById("save").disabled = false;
notify.style.color = "red";
alert("При валидации данных произошла ошибка");
} else {
console.log(tuning_json);

let data = JSON.stringify(tuning_json);
let xhr = new XMLHttpRequest();
xhr.open("POST", "/tuning", true);
xhr.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
xhr.setRequestHeader("Content-Type","application/json; charset=utf-8");
xhr.timeout = 10000;
xhr.onload = function() {
if (xhr.status != 200) {
document.getElementById("notice").textContent = "Настройки не сохранены.";
console.log("Settings data not transferred.");
console.log(xhr.status + " : " + xhr.statusText);
document.getElementById("save").disabled = false;
} else {
document.getElementById("notice").textContent = "Настройки сохранены.";
//setTimeout(function() { window.location.href = '/';}, 10000);
console.log("Settings data transferred.");
}
};
xhr.onerror = function() {
console.log("Request failed");
};
xhr.send(data);
delete xhr;
}
}

function Set1() {
let settings_json = JSON.parse(window.sessionStorage.getItem("settings"));

document.getElementById("version").innerHTML = "Версия ПО "+settings_json.version;
document.getElementById("serial").innerHTML = "Серийный № "+n(settings_json.serial, 4);
document.getElementById("mac").innerHTML = "MAC 00:10:A1:71:"+n(settings_json.mac1, 2)+":"+n(settings_json.mac0, 2);

let min = settings_json.uptime / 60;
let hour = min / 60;
let out_time = n(Math.floor(hour / 24), 3)  +' д ' +  n(Math.floor(hour % 24), 2)  +' ч ' +
n(Math.floor(min % 60), 2) +' м ' + n(Math.floor(settings_json.uptime % 60), 2) + " c";

document.getElementById("uptime").textContent = out_time;
}

function Set2() {
let tuning_json = JSON.parse(window.sessionStorage.getItem("tuning"));

document.getElementById("hysteresis_window").value = (tuning_json.hysteresis_window / 100).toFixed(2);
document.getElementById("self_test_duration").value = tuning_json.self_test_duration;
document.getElementById("min_stroke_rod").value = tuning_json.min_stroke_rod;
document.getElementById("min_stroke").value = tuning_json.min_stroke;
document.getElementById("adc_min_stroke").value = (tuning_json.adc_min_stroke / 100).toFixed(2);
document.getElementById("max_stroke").value = tuning_json.max_stroke;
document.getElementById("adc_max_stroke").value = (tuning_json.adc_max_stroke / 100).toFixed(2);
document.getElementById("zero_offset").value = tuning_json.zero_offset;
document.getElementById("debounce_in").value = tuning_json.debounce_in;
document.getElementById("delay_out").value = tuning_json.delay_out;
}

async function State1() {
let xhr = new XMLHttpRequest();
xhr.open("GET", "/set0.json");
xhr.responseType = "json";
xhr.setRequestHeader("Content-Type", "application/json; charset=utf-8");
xhr.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
xhr.timeout = 2000;
xhr.onload = function() {
if (xhr.status != 200) {
console.log("No response received");
console.log(xhr.status + " : " + xhr.statusText);
} else {
console.log("Response received");
let resp = xhr.response;
window.sessionStorage.removeItem("settings");
window.sessionStorage.setItem("settings", JSON.stringify(resp));
Set1()
}
};
xhr.onerror = function() {
console.log("Request failed");
};
xhr.send();
delete xhr;
}

async function State2() {
let xhr = new XMLHttpRequest();
xhr.open("GET", "/set1.json");
xhr.responseType = "json";
xhr.setRequestHeader("Content-Type", "application/json; charset=utf-8");
xhr.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
xhr.timeout = 2000;
xhr.onload = function() {
if (xhr.status != 200) {
console.log("No response received");
console.log(xhr.status + " : " + xhr.statusText);
} else {
console.log("Response received");
let resp = xhr.response;
window.sessionStorage.removeItem("tuning");
window.sessionStorage.setItem("tuning", JSON.stringify(resp));
Set2();
}
};
xhr.onerror = function() {
console.log("Request failed");
};
xhr.send();
delete xhr;
}

window.onload = function() {
State1();
State2();
}