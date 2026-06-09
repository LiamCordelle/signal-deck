var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

var STORAGE_KEY = 'signalDeckSettings';
var CLAY_STORAGE_KEY = 'clay-settings';

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

var settings = loadSettings();
syncClaySettings();

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
  var text;
  if (typeof value === 'number' && isFinite(value) && value >= 0 && value <= 0xffffff) {
    text = value.toString(16);
    while (text.length < 6) {
      text = '0' + text;
    }
    return '#' + text.toLowerCase();
  }
  if (typeof value === 'string') {
    text = value.replace(/^#|^0x/, '');
    if (/^[0-9a-fA-F]{6}$/.test(text)) {
      return '#' + text.toLowerCase();
    }
  }
  return fallback;
}

function colorToInt(value) {
  return parseInt(value.replace('#', ''), 16);
}

function flattenClaySettings(raw) {
  var output = {};
  var key;
  var item;
  for (key in raw) {
    if (raw.hasOwnProperty(key)) {
      item = raw[key];
      output[key] = item && typeof item === 'object' &&
          item.hasOwnProperty('value') ? item.value : item;
    }
  }
  return output;
}

function claySettingsFromSettings(value) {
  return {
    darkMode: value.darkMode,
    timeMode: value.timeMode,
    tempUnit: value.tempUnit,
    stepGoal: value.stepGoal,
    userAge: value.userAge,
    world: value.world,
    hrColor: value.hrColor,
    stepsColor: value.stepsColor,
    rainColor: value.rainColor,
    footerLeft: value.footerLeft,
    footerCenter: value.footerCenter,
    footerRight: value.footerRight,
    demoFallback: value.demoFallback,
    showWeather: value.showWeather,
    showHealth: value.showHealth,
    showBatteryPercent: value.showBatteryPercent
  };
}

function syncClaySettings() {
  try {
    clay.setSettings(claySettingsFromSettings(settings));
  } catch (e) {
    console.log('Clay settings sync failed: ' + e);
  }
}

function loadSettings() {
  try {
    var clayRaw = localStorage.getItem(CLAY_STORAGE_KEY);
    if (clayRaw) {
      return copyDefaults(flattenClaySettings(JSON.parse(clayRaw)));
    }
  } catch (e) {
    console.log('Clay settings load failed: ' + e);
  }
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
  syncClaySettings();
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

Pebble.addEventListener('ready', function() {
  sendSettings();
  getWeather();
});

Pebble.addEventListener('showConfiguration', function() {
  syncClaySettings();
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) {
    return;
  }
  try {
    settings = copyDefaults(flattenClaySettings(clay.getSettings(e.response, false)));
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
