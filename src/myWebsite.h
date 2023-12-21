const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<style type="text/css">
.button {
  background-color: #4CAF50; /* Green */
  border: none;
  color: white;
  padding: 1px 22px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
input {
  margin: 0.4rem;
}
</style>
<body style="background-color: #f9e79f ">
<center>
<div>
<h1>ESP8266 WEBSERVER</h1>
<BR>
</div>
<br>  
<div><h2>
  Leistung (W): <span id="wattage">?</span><br><br>
  Temperatur (&deg;C): <span id="temperature">?</span><br><br> 
  Max.-Temp. (&deg;C): <span id="maxTemp">?</span><br><br> 
  LED State: <span id="ledState">?</span> <button class="button" onclick="send(1)">LED ON</button>
<button class="button" onclick="send(0)">LED OFF</button><br><br>
  Automatikmodus: <span id="modeAuto">?</span> 
    <button class="button" onclick="setModeAuto(1)">an</button>
    <button class="button" onclick="setModeAuto(0)">aus</button><br><br>
  <div>
  FAN Speed: <span id="fanSpeed">?</span>
  <input type="range" id="fanSpeed_range" name="fanSpeed" min="0" max="255" value="0" step="5" oninput="changeValue(this.value)" onchange = "setFanSpeed(this.value)"/>
  </div><br><br>
  Hostname: <input id="hostname" type="text" list="dliste" value="" /> <button class="button" onclick="setHostname()">setzen</button>
</h2>
</div>
<script>
  let fanSpeed = document.getElementById('fanSpeed');
  function changeValue(newVal) {
    fanSpeed.innerHTML = newVal;
  }

function send(led_sts) 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("state").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "led_set?state="+led_sts, true);
  xhttp.send();
}

function setHostname() 
{
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "hostname_set?new_hostname="+document.getElementById("hostname").value, true);
  xhttp.send();
}

function setModeAuto(modeAuto_state) 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("modeAuto").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "modeAuto_set?modeAuto="+modeAuto_state, true);
  xhttp.send();
}

function setFanSpeed(fanSpeed_state) 
{
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", "fanSpeed_set?fanSpeed="+fanSpeed_state, true);
  xhttp.send();
  fanSpeed.innerHTML = "???";
}

setInterval(function() { getServerVar("wattage"); }, 2000);
setInterval(function() { getServerVar("temperature"); }, 2000);
setInterval(function() {  getServerVar("fanSpeed"); 
                          document.getElementById("fanSpeed_range").value = document.getElementById("fanSpeed").innerHTML
                        }, 2000);

setTimeout(function() { getServerVar("ledState"); }, 800);
setTimeout(function() { getServerVar("modeAuto"); }, 900);
setTimeout(function() { getServerVar("maxTemp"); }, 1000);
setTimeout(function() {  getServerVarValue("hostname"); }, 1100);

// serverVar needs to be Readable by a method called <varName>_get
function getServerVar(varName) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {      
      document.getElementById(varName).innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", varName + "_get", true);
  xhttp.send();
}

// serverVar needs to be Readable by a method called <varName>_get
function getServerVarValue(varName) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {      
      document.getElementById(varName).value =
      this.responseText;
    }
  };
  xhttp.open("GET", varName + "_get", true);
  xhttp.send();
}

</script>
</center>
</body>
</html>
)=====";