const btn = document.getElementById("connectBtn");
const canvas = document.getElementById("ecg");
const ctx = canvas.getContext("2d");
const statusText = document.getElementById("statusText");
const heart = document.getElementById("heart");

let port, reader;
let buffer = "";
let data = [];

// Beat detection
let lastValue = 0;
let beatDetected = false;
let lastBeatTime = 0;

// Finger detection
let lastFingerTime = 0;
const fingerTimeout = 2000; // ms

ctx.strokeStyle = "#f1c40f";
ctx.lineWidth = 2;

btn.onclick = async () => {
  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 9600 });

    const decoder = new TextDecoder();
    reader = port.readable.getReader();

    statusText.textContent = "Connected – Waiting for Finger";
    stopHeart();
    readLoop(decoder);
  } catch (err) {
    statusText.textContent = "Connection Failed";
    console.error(err);
  }
};

async function readLoop(decoder) {
  while (true) {
    const { value, done } = await reader.read();
    if (done) break;

    buffer += decoder.decode(value, { stream: true });
    let lines = buffer.split("\n");
    buffer = lines.pop();

    for (let line of lines) {
      let v = parseFloat(line.trim());
      if (!isNaN(v)) {
        data.push(v);
        if (data.length > 800) data.shift();
        handleFinger(v);
        detectBeat(v);
        drawECG();
      }
    }
  }
}

function drawECG() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.beginPath();

  data.forEach((v, i) => {
    let x = (i / data.length) * canvas.width;
    let y = canvas.height - (v / 1000) * canvas.height;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });

  ctx.stroke();
}

// Detect finger presence
function handleFinger(v) {
  const fingerThreshold = 450;

  if (v > fingerThreshold) {
    lastFingerTime = Date.now();
    statusText.textContent = "Live Signal Detected";
  } else {
    if (Date.now() - lastFingerTime > fingerTimeout) {
      statusText.textContent = "Waiting for Finger";
      stopHeart();
    }
  }
}

// Beat detection
function detectBeat(v) {
  const threshold = 450;
  const minInterval = 300;

  if (v > threshold && lastValue <= threshold && !beatDetected) {
    beatDetected = true;
    const now = Date.now();

    if (lastBeatTime > 0) {
      const interval = now - lastBeatTime;
      if (interval > minInterval && interval < 2000) {
        const bpm = Math.round(60000 / interval);
        startHeart(bpm);
      }
    }

    lastBeatTime = now;
  }

  if (v < threshold) {
    beatDetected = false;
  }

  lastValue = v;
}

// Heart controls
function startHeart(bpm) {
  heart.style.animationPlayState = "running";
  heart.style.animationDuration = (60 / bpm) + "s";
}

function stopHeart() {
  heart.style.animationPlayState = "paused";
}
