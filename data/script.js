var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var myTogglePending = false;
var wasRecording = false;
var lastMessageTime = Date.now();
var heartbeatInterval;

window.addEventListener('load', onLoad);

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
  lastMessageTime = Date.now();
  
  clearInterval(heartbeatInterval);
  heartbeatInterval = setInterval(function() {
    if (Date.now() - lastMessageTime > 5000) {
      console.log('No data for 5s - reconnecting...');
      websocket.close();
    }
  }, 3000);
}

function onClose(event) {
  console.log('Connection closed');
  clearInterval(heartbeatInterval);
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  lastMessageTime = Date.now();
  const data = JSON.parse(event.data);

  document.getElementById("temperature").innerHTML = data.temperature.toFixed(2);
  document.getElementById("altitude").innerHTML = data.altitude.toFixed(2);
  document.getElementById("pressure").innerHTML = data.pressure.toFixed(2);
  document.getElementById("realData").innerHTML = data.realData ? "BMP280" : "Fake";
  document.getElementById("clients").innerHTML = data.clients;

  const button = document.getElementById("Startbutton");
  const stateText = document.getElementById("state");
  const stateContainer = document.getElementById("stateContainer");

  if (data.recording) {
    button.innerHTML = "Stop";
    button.classList.add("recording");
    stateText.innerHTML = "RECORDING!";
    stateContainer.classList.add("recording");
    myTogglePending = false;
  } else {
    button.innerHTML = "Start";
    button.classList.remove("recording");
    stateText.innerHTML = "not recording...";
    stateContainer.classList.remove("recording");

    if (myTogglePending && wasRecording) {
      myTogglePending = false;
      window.location.href = '/download';
    }
  }

  wasRecording = data.recording;
}

function onLoad(event) {
  initWebSocket();
  initButton();
  
  document.addEventListener('visibilitychange', function() {
    if (document.visibilityState === 'visible') {
      if (!websocket || websocket.readyState !== WebSocket.OPEN) {
        console.log('Page visible again - reconnecting...');
        initWebSocket();
      }
    }
  });
}

function initButton() {
  document.getElementById('Startbutton').addEventListener('click', toggle);
}

function toggle() {
  myTogglePending = true;
  websocket.send('toggle');
}
