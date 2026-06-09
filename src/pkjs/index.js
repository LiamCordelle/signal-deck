var STORAGE_KEY = 'signalDeckSettings';

var DEFAULT_SETTINGS = {
  darkMode: false,
  timeMode: 0,
  tempUnit: 0,
  stepGoal: 8000,
  userAge: 30,
  world: 'nz',
  hrColor: '#ff7393',
  stepsColor: '#00b96d',
  rainColor: '#009bff',
  footerLeft: 0,
  footerCenter: 1,
  footerRight: 2,
  demoFallback: true,
  showWeather: true,
  showHealth: true,
  showBatteryPercent: true
};

var WORLD_OPTIONS = {
  nz: { label: 'NZ', offset: 720, dst: 1 },
  utc: { label: 'UTC', offset: 0, dst: 0 },
  lon: { label: 'LON', offset: 0, dst: 2 },
  nyc: { label: 'NYC', offset: -300, dst: 3 },
  la: { label: 'LA', offset: -480, dst: 3 },
  tky: { label: 'TKY', offset: 540, dst: 0 },
  syd: { label: 'SYD', offset: 600, dst: 4 }
};

var FOOTER_OPTIONS = [
  ['UV index', 0],
  ['Sun event', 1],
  ['World clock', 2],
  ['Battery', 3],
  ['Weather', 4],
  ['Temperature', 5],
  ['Rain chance', 6],
  ['Steps', 7],
  ['Heart rate', 8]
];

var settings = loadSettings();

var xhrRequest = function(url, type, callback, errorCallback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    if (xhr.status >= 200 && xhr.status < 300) {
      callback(this.responseText);
    } else if (errorCallback) {
      errorCallback('HTTP ' + xhr.status);
    }
  };
  xhr.onerror = function() {
    if (errorCallback) {
      errorCallback('Network error');
    }
  };
  xhr.open(type, url);
  xhr.send();
};

function copyDefaults(target) {
  var output = {};
  var key;
  for (key in DEFAULT_SETTINGS) {
    if (DEFAULT_SETTINGS.hasOwnProperty(key)) {
      output[key] = DEFAULT_SETTINGS[key];
    }
  }
  if (target) {
    for (key in target) {
      if (target.hasOwnProperty(key)) {
        output[key] = target[key];
      }
    }
  }
  return normalizeSettings(output);
}

function normalizeSettings(value) {
  value.darkMode = !!value.darkMode;
  value.timeMode = numberInRange(value.timeMode, 0, 2, DEFAULT_SETTINGS.timeMode);
  value.tempUnit = numberInRange(value.tempUnit, 0, 1, DEFAULT_SETTINGS.tempUnit);
  value.stepGoal = numberInRange(value.stepGoal, 1000, 50000, DEFAULT_SETTINGS.stepGoal);
  value.userAge = numberInRange(value.userAge, 10, 100, DEFAULT_SETTINGS.userAge);
  if (!WORLD_OPTIONS[value.world]) {
    value.world = DEFAULT_SETTINGS.world;
  }
  value.hrColor = normalizeColor(value.hrColor, DEFAULT_SETTINGS.hrColor);
  value.stepsColor = normalizeColor(value.stepsColor, DEFAULT_SETTINGS.stepsColor);
  value.rainColor = normalizeColor(value.rainColor, DEFAULT_SETTINGS.rainColor);
  value.footerLeft = numberInRange(value.footerLeft, 0, 8, DEFAULT_SETTINGS.footerLeft);
  value.footerCenter = numberInRange(value.footerCenter, 0, 8, DEFAULT_SETTINGS.footerCenter);
  value.footerRight = numberInRange(value.footerRight, 0, 8, DEFAULT_SETTINGS.footerRight);
  value.demoFallback = value.demoFallback !== false;
  value.showWeather = value.showWeather !== false;
  value.showHealth = value.showHealth !== false;
  value.showBatteryPercent = value.showBatteryPercent !== false;
  return value;
}

function numberInRange(value, min, max, fallback) {
  var parsed = parseInt(value, 10);
  if (isNaN(parsed)) {
    parsed = fallback;
  }
  if (parsed < min) {
    return min;
  }
  if (parsed > max) {
    return max;
  }
  return parsed;
}

function normalizeColor(value, fallback) {
  if (typeof value !== 'string' || !/^#[0-9a-fA-F]{6}$/.test(value)) {
    return fallback;
  }
  return value.toLowerCase();
}

function colorToInt(value) {
  return parseInt(value.replace('#', ''), 16);
}

function loadSettings() {
  try {
    var raw = localStorage.getItem(STORAGE_KEY);
    if (raw) {
      return copyDefaults(JSON.parse(raw));
    }
  } catch (e) {
    console.log('Settings load failed: ' + e);
  }
  return copyDefaults(null);
}

function saveSettings() {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
  } catch (e) {
    console.log('Settings save failed: ' + e);
  }
}

function settingsDictionary() {
  var world = WORLD_OPTIONS[settings.world] || WORLD_OPTIONS.nz;
  return {
    'DARK_MODE': settings.darkMode ? 1 : 0,
    'TIME_MODE': settings.timeMode,
    'TEMP_UNIT': settings.tempUnit,
    'STEP_GOAL': settings.stepGoal,
    'USER_AGE': settings.userAge,
    'WORLD_LABEL': world.label,
    'WORLD_OFFSET_MIN': world.offset,
    'WORLD_DST_MODE': world.dst,
    'HR_COLOR': colorToInt(settings.hrColor),
    'STEPS_COLOR': colorToInt(settings.stepsColor),
    'RAIN_COLOR': colorToInt(settings.rainColor),
    'FOOTER_LEFT': settings.footerLeft,
    'FOOTER_CENTER': settings.footerCenter,
    'FOOTER_RIGHT': settings.footerRight,
    'DEMO_FALLBACK': settings.demoFallback ? 1 : 0,
    'SHOW_WEATHER': settings.showWeather ? 1 : 0,
    'SHOW_HEALTH': settings.showHealth ? 1 : 0,
    'SHOW_BATTERY_PERCENT': settings.showBatteryPercent ? 1 : 0
  };
}

function sendSettings() {
  Pebble.sendAppMessage(settingsDictionary(),
    function() { console.log('Settings sent'); },
    function() { console.log('Settings send failed'); }
  );
}

function firstNumber(arrayValue, fallback) {
  if (arrayValue && arrayValue.length && typeof arrayValue[0] === 'number') {
    return arrayValue[0];
  }
  return fallback;
}

function formatIsoTime(value) {
  if (!value || value.length < 16) {
    return '--:--';
  }
  return value.substring(11, 16);
}

function minutesFromIso(value) {
  if (!value || value.length < 16) {
    return -1;
  }
  return parseInt(value.substring(11, 13), 10) * 60 +
      parseInt(value.substring(14, 16), 10);
}

function chooseNextSunEvent(currentTime, daily) {
  var nowMinutes = minutesFromIso(currentTime);
  var sunrise = daily.sunrise || [];
  var sunset = daily.sunset || [];

  if (nowMinutes < 0 || sunrise.length < 1 || sunset.length < 1) {
    return { time: '--:--', type: -1 };
  }

  var riseToday = minutesFromIso(sunrise[0]);
  var setToday = minutesFromIso(sunset[0]);

  if (riseToday >= 0 && nowMinutes < riseToday) {
    return { time: formatIsoTime(sunrise[0]), type: 0 };
  }
  if (setToday >= 0 && nowMinutes < setToday) {
    return { time: formatIsoTime(sunset[0]), type: 1 };
  }
  if (sunrise.length > 1) {
    return { time: formatIsoTime(sunrise[1]), type: 0 };
  }
  return { time: formatIsoTime(sunrise[0]), type: 0 };
}

function sendWeatherError() {
  Pebble.sendAppMessage({
    'WEATHER_CODE': -1
  });
}

function locationSuccess(pos) {
  var url = 'https://api.open-meteo.com/v1/forecast?' +
      'latitude=' + encodeURIComponent(pos.coords.latitude) +
      '&longitude=' + encodeURIComponent(pos.coords.longitude) +
      '&current=temperature_2m,weather_code' +
      '&hourly=precipitation_probability,uv_index' +
      '&daily=sunrise,sunset' +
      '&forecast_hours=1' +
      '&forecast_days=2' +
      '&timezone=auto';

  xhrRequest(url, 'GET', function(responseText) {
    var json = JSON.parse(responseText);
    var current = json.current || {};
    var hourly = json.hourly || {};
    var daily = json.daily || {};
    var sunEvent = chooseNextSunEvent(current.time, daily);

    var dictionary = {
      'TEMPERATURE': Math.round(current.temperature_2m || 0),
      'WEATHER_CODE': Math.round(current.weather_code || 0),
      'RAIN_CHANCE': Math.round(firstNumber(hourly.precipitation_probability, 0)),
      'UV_INDEX': Math.round(firstNumber(hourly.uv_index, 0) * 10),
      'SUN_EVENT_TIME': sunEvent.time,
      'SUN_EVENT_TYPE': sunEvent.type
    };

    Pebble.sendAppMessage(dictionary,
      function() { console.log('Signal Deck weather sent'); },
      function() { console.log('Signal Deck weather send failed'); }
    );
  }, function(error) {
    console.log('Weather request failed: ' + error);
    sendWeatherError();
  });
}

function locationError(err) {
  console.log('Location failed: ' + err.code);
  sendWeatherError();
}

function getWeather() {
  if (!settings.showWeather) {
    return;
  }
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    { timeout: 15000, maximumAge: 300000 }
  );
}

function selected(value, current) {
  return parseInt(value, 10) === parseInt(current, 10) ? ' selected' : '';
}

function checked(value) {
  return value ? ' checked' : '';
}

function footerOptions(current) {
  var html = '';
  for (var i = 0; i < FOOTER_OPTIONS.length; i++) {
    html += '<option value="' + FOOTER_OPTIONS[i][1] + '"' +
        selected(FOOTER_OPTIONS[i][1], current) + '>' + FOOTER_OPTIONS[i][0] +
        '</option>';
  }
  return html;
}

function worldOptions(current) {
  var labels = [
    ['nz', 'New Zealand'],
    ['utc', 'UTC'],
    ['lon', 'London'],
    ['nyc', 'New York'],
    ['la', 'Los Angeles'],
    ['tky', 'Tokyo'],
    ['syd', 'Sydney']
  ];
  var html = '';
  for (var i = 0; i < labels.length; i++) {
    html += '<option value="' + labels[i][0] + '"' +
        (labels[i][0] === current ? ' selected' : '') + '>' + labels[i][1] +
        '</option>';
  }
  return html;
}

function configPageHtml() {
  return '<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">' +
    '<style>body{font-family:-apple-system,BlinkMacSystemFont,Helvetica,Arial,sans-serif;margin:0;background:#f8f0dc;color:#081827}' +
    'main{padding:16px}h1{font-size:20px;margin:0 0 12px}.section{border-top:1px solid #c8c1af;padding:14px 0}' +
    'label{display:block;font-size:13px;font-weight:700;margin:12px 0 4px}select,input{box-sizing:border-box;width:100%;font-size:16px;padding:8px;border:1px solid #7d877d;border-radius:4px;background:#fff}' +
    'input[type=checkbox]{width:auto;margin-right:8px}.check{display:flex;align-items:center;margin:10px 0;font-weight:700}' +
    '.row{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}.row label{margin-top:0}' +
    'button{width:100%;padding:12px;background:#081827;color:white;border:0;border-radius:4px;font-size:16px;font-weight:800}</style></head>' +
    '<body><main><h1>Signal Deck</h1>' +
    '<div class="section">' +
    '<label class="check"><input id="darkMode" type="checkbox"' + checked(settings.darkMode) + '>Dark mode</label>' +
    '<label>Time format</label><select id="timeMode">' +
    '<option value="0"' + selected(0, settings.timeMode) + '>System</option>' +
    '<option value="1"' + selected(1, settings.timeMode) + '>12-hour</option>' +
    '<option value="2"' + selected(2, settings.timeMode) + '>24-hour</option></select>' +
    '<label>Temperature</label><select id="tempUnit">' +
    '<option value="0"' + selected(0, settings.tempUnit) + '>Celsius</option>' +
    '<option value="1"' + selected(1, settings.tempUnit) + '>Fahrenheit</option></select>' +
    '</div><div class="section">' +
    '<label>Step goal</label><input id="stepGoal" type="number" min="1000" max="50000" step="500" value="' + settings.stepGoal + '">' +
    '<label>User age</label><input id="userAge" type="number" min="10" max="100" value="' + settings.userAge + '">' +
    '<label>World clock</label><select id="world">' + worldOptions(settings.world) + '</select>' +
    '</div><div class="section">' +
    '<label>Heart accent</label><input id="hrColor" type="color" value="' + settings.hrColor + '">' +
    '<label>Steps accent</label><input id="stepsColor" type="color" value="' + settings.stepsColor + '">' +
    '<label>Rain accent</label><input id="rainColor" type="color" value="' + settings.rainColor + '">' +
    '</div><div class="section"><div class="row">' +
    '<label>Left<select id="footerLeft">' + footerOptions(settings.footerLeft) + '</select></label>' +
    '<label>Center<select id="footerCenter">' + footerOptions(settings.footerCenter) + '</select></label>' +
    '<label>Right<select id="footerRight">' + footerOptions(settings.footerRight) + '</select></label>' +
    '</div></div><div class="section">' +
    '<label class="check"><input id="demoFallback" type="checkbox"' + checked(settings.demoFallback) + '>Use demo values when sensors are empty</label>' +
    '<label class="check"><input id="showWeather" type="checkbox"' + checked(settings.showWeather) + '>Show weather</label>' +
    '<label class="check"><input id="showHealth" type="checkbox"' + checked(settings.showHealth) + '>Show health gauges</label>' +
    '<label class="check"><input id="showBatteryPercent" type="checkbox"' + checked(settings.showBatteryPercent) + '>Show battery percentage</label>' +
    '</div><button onclick="save()">Save</button></main>' +
    '<script>function q(n,d){var p=location.search.substring(1).split("&");for(var i=0;i<p.length;i++){var kv=p[i].split("=");if(decodeURIComponent(kv[0]||"")===n){return decodeURIComponent(kv.slice(1).join("=")||"")}}return d}' +
    'function v(id){return document.getElementById(id).value}function c(id){return document.getElementById(id).checked}' +
    'function save(){var s={darkMode:c("darkMode"),timeMode:parseInt(v("timeMode"),10),tempUnit:parseInt(v("tempUnit"),10),' +
    'stepGoal:parseInt(v("stepGoal"),10),userAge:parseInt(v("userAge"),10),world:v("world"),hrColor:v("hrColor"),stepsColor:v("stepsColor"),rainColor:v("rainColor"),' +
    'footerLeft:parseInt(v("footerLeft"),10),footerCenter:parseInt(v("footerCenter"),10),footerRight:parseInt(v("footerRight"),10),' +
    'demoFallback:c("demoFallback"),showWeather:c("showWeather"),showHealth:c("showHealth"),showBatteryPercent:c("showBatteryPercent")};' +
    'location.href=q("return_to","pebblejs://close#")+encodeURIComponent(JSON.stringify(s));}</script></body></html>';
}

Pebble.addEventListener('ready', function() {
  sendSettings();
  getWeather();
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL('data:text/html;charset=utf-8,' + encodeURIComponent(configPageHtml()));
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) {
    return;
  }
  try {
    settings = copyDefaults(JSON.parse(decodeURIComponent(e.response)));
    saveSettings();
    sendSettings();
    getWeather();
  } catch (err) {
    console.log('Config parse failed: ' + err);
  }
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload['REQUEST_SETTINGS']) {
    sendSettings();
  }
  if (e.payload['REQUEST_WEATHER']) {
    getWeather();
  }
});
