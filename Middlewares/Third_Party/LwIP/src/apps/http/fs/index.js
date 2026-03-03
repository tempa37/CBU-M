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
window.sessionStorage.setItem("settings", JSON.stringify(resp))
Set();
}
};
xhr.onerror = function() {
console.log("Request failed");
};
xhr.send();
delete xhr;
}

function LoadFile() {
let upload = document.getElementById("fileinput");
if (upload) {
upload.addEventListener("change", function() {
if (upload.files.length > 0) {
let reader = new FileReader();
reader.addEventListener('load', function() {
// convert string to json object
let settings_json = JSON.parse(reader.result);
let file_status = 0;
if ("usk_ip" in settings_json) {
file_status++;
}
if ("usk_mask" in settings_json) {
file_status++;
}
if ("usk_gateway" in settings_json) {
file_status++;
}
if (file_status == 3) {
window.sessionStorage.setItem("settings", JSON.stringify(settings_json))
Set();
} else {
console.log("This not settings file");
}
});
reader.readAsText(upload.files[0]);
}
});
}
}

function SaveFile() {
let data = window.sessionStorage.getItem("settings");
let blob = new Blob([data], { type: "application/json" });
let url = URL.createObjectURL(blob);
let a = document.createElement("a");
a.href = url;
a.download = "settings.json";
a.click();
URL.revokeObjectURL(url);
}

function SendSettings() {
document.getElementById("save").disabled = true;
let ip_pattern = "^((1?[\\d]?[\\d]|2([0-4][\\d]|5[0-5]))[.]){3}(1?[\\d]?[\\d]|2([0-4][\\d]|5[0-5]))$";
let re = new RegExp(ip_pattern);
let validate_status = 0;
let settings_json = JSON.parse(window.sessionStorage.getItem("settings"));

let notify = document.getElementById("notice");
notify.style.color = "black";
notify.textContent = "";

const elem = document.getElementsByTagName("input");

for (let i = 0; i < elem.length; i++) {
const item = elem[i];
if (item.name == "ip") {
if (!re.test(item.value)) {
validate_status++;
notify.innerHTML += "Проверьте IP адреса<br>";
} else {
settings_json[item.id] = item.value;
}
} else if (item.name == "id") {
if (item.value < 1 || item.value > 247) {
validate_status++;
notify.innerHTML += "Проверьте Modbus адреса<br>";
} else {
settings_json[item.id] = item.value;
}
} else if (item.name == "inout") {
if (item.value < 1 || item.value > 8) {
validate_status++;
notify.innerHTML += "Проверьте номера Вх-Вых<br>";
} else {
settings_json[item.id] = item.value;
}
} else if (item.name == "val") {
if (item.id == "sens2_min_val" || item.id == "sens2_max_val") {
settings_json[item.id] = Math.round(parseFloat(item.value) * 100);
} else {
settings_json[item.id] = item.value;
}
}
if (item.id == "mb_rate") {
let val = parseInt(item.value);
if (val < 100 || val > 500) {
validate_status++;
notify.innerHTML += "Величина времени находится за границами рабочего диапазона<br>";
} else {
settings_json[item.id] = item.value;
}
}
if (item.id == "mb_timeout") {
let val = parseInt(item.value);
if (val < 50 || val > 250) {
validate_status++;
notify.innerHTML += "Величина времени находится за границами рабочего диапазона<br>";
} else {
settings_json[item.id] = item.value;
}
}

//if(item.type == "text") {
//    console.log(i +"\t" +item.id+"\t" +item.name+"\t" +item.value);
//}
}
if (parseFloat(settings_json.sens2_min_val) >= parseFloat(settings_json.sens2_max_val)) {
notify.innerHTML += "min больше max 1<br>";
validate_status++;
}
if (parseFloat(settings_json.sens3_min_lim) >= parseFloat(settings_json.sens3_max_lim)) {
notify.innerHTML += "min больше max 2<br>";
validate_status++;
}
if (parseFloat(settings_json.sens3_min_val) >= parseFloat(settings_json.sens3_max_val)) {
notify.innerHTML += "min больше max 3<br>";
validate_status++;
}
if (parseInt(settings_json.valve1_io) == parseInt(settings_json.valve2_io) ||
parseInt(settings_json.valve1_io) == parseInt(settings_json.state_out_io) ||
parseInt(settings_json.valve2_io) == parseInt(settings_json.state_out_io))
{
notify.innerHTML += `Одинаковые номера выходов ${settings_json.valve1_io}, ${settings_json.valve2_io}, ${settings_json.state_out_io}<br>`;
validate_status++;
}
if (parseInt(settings_json.sens3_io) == parseInt(settings_json.state_in_io)) {
notify.innerHTML += `Одинаковые номера входов ${settings_json.sens3_io}, ${settings_json.state_in_io}<br>`;
validate_status++;
}
if (parseInt(settings_json.sens1_id) == parseInt(settings_json.sens2_id)) {
notify.innerHTML += "Одинаковые адреса 'Магистраль' и 'Цилиндр'<br>";
validate_status++;
}
if (validate_status > 0) {
document.getElementById("save").disabled = false;
notify.style.color = "red";
alert("При валидации данных произошла ошибка");
} else {
console.log(settings_json);
let data = JSON.stringify(settings_json);
let xhr = new XMLHttpRequest();
xhr.open("POST", "/settings", true);
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
setTimeout(function() { window.location.href = '/';}, 10000);
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

function n(num, len) {
return `${num}`.padStart(len, '0');
}

async function Set() {
let settings_json = JSON.parse(window.sessionStorage.getItem("settings"));

let ip_pattern = "^((1?[\\d]?[\\d]|2([0-4][\\d]|5[0-5]))[.]){3}(1?[\\d]?[\\d]|2([0-4][\\d]|5[0-5]))$";
let id_pattern="^(2[0-4][0-7]|1[0-9][0-9]|[0-9][0-9]|[1-9]?)$";
let inout_pattern="^([1-8]?)$";

const elem = document.getElementsByTagName("input");
for (let i = 0; i < elem.length; i++) {
const item = elem[i];
if (item.name == "ip") {
item.pattern = ip_pattern;
} else if (item.name == "id") {
item.pattern = id_pattern;
} else if (item.name == "inout") {
item.pattern = inout_pattern;
}
}

// 0
document.getElementById("version").innerHTML = "Версия ПО "+settings_json.version;

document.getElementById("serial").innerHTML = "Серийный № "+n(settings_json.serial, 4);

document.getElementById("mac").innerHTML = "MAC 00:10:A1:71:"+n(settings_json.mac1, 2)+":"+n(settings_json.mac0, 2);

let min = settings_json.uptime / 60;
let hour = min / 60;
let out_time = n(Math.floor(hour / 24), 3)  +' д ' +  n(Math.floor(hour % 24), 2)  +' ч ' +
n(Math.floor(min % 60), 2) +' м ' + n(Math.floor(settings_json.uptime % 60), 2) + " c";

document.getElementById("uptime").textContent = out_time;
// 1
document.getElementById("usk_ip").value = settings_json.usk_ip;

document.getElementById("usk_mask").value = settings_json.usk_mask;

document.getElementById("usk_gateway").value = settings_json.usk_gateway;

document.getElementById("self_id").value = settings_json.self_id;

document.getElementById("mb_rate").value = settings_json.mb_rate;

document.getElementById("mb_timeout").value = settings_json.mb_timeout;
// 2
if (document.getElementById("sens1_id") != null) {
document.getElementById("sens1_id").value = settings_json.sens1_id;
document.getElementById("sens2_id").value = settings_json.sens2_id;
document.getElementById("sens2_min_val").value = (settings_json.sens2_min_val / 100).toFixed(2);
document.getElementById("sens2_max_val").value = (settings_json.sens2_max_val / 100).toFixed(2);
}
// 3
if (document.getElementById("sens3_min_lim") != null) {
document.getElementById("sens3_min_lim").value = settings_json.sens3_min_lim;
document.getElementById("sens3_max_lim").value = settings_json.sens3_max_lim;
document.getElementById("sens3_min_val").value = settings_json.sens3_min_val;
document.getElementById("sens3_max_val").value = settings_json.sens3_max_val;
}
// 4
if (document.getElementById("urm_id") != null) {
document.getElementById("urm_id").value = settings_json.urm_id;
document.getElementById("valve1_io").value = settings_json.valve1_io;
document.getElementById("valve2_io").value = settings_json.valve2_io;
document.getElementById("state_out_io").value = settings_json.state_out_io;
}

// 5
document.getElementById("umvh_id").value = settings_json.umvh_id;
if (document.getElementById("sens3_io") != null) {
document.getElementById("sens3_io").value = settings_json.sens3_io;
}
document.getElementById("state_in_io").value = settings_json.state_in_io;
}

function StartUartTest() {
let btn = document.getElementById("uart-test-btn");
if (btn != null) {
btn.disabled = true;
}
let xhr = new XMLHttpRequest();
xhr.open("GET", "/uart_test.json", true);
xhr.timeout = 3000;
xhr.onload = function() {
if (btn != null) {
btn.disabled = false;
}
if (xhr.status == 200 && xhr.responseText.length > 0) {
let data = {};
try {
data = JSON.parse(xhr.responseText);
} catch (e) {
alert("UART тест: некорректный ответ");
return;
}
let msg = "UART1->UART2: " + (data.uart1_to_uart2 ? "OK" : "FAIL") + "\n" +
"UART2->UART1: " + (data.uart2_to_uart1 ? "OK" : "FAIL");
if (data.ok == 1) {
alert("UART тест пройден\n" + msg);
} else {
alert("UART тест не пройден\n" + msg);
}
} else {
alert("Не удалось выполнить UART тест");
}
};
xhr.onerror = function() {
if (btn != null) {
btn.disabled = false;
}
alert("Ошибка запроса UART теста");
};
xhr.send();
}


