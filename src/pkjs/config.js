var FOOTER_OPTIONS = [
  { label: 'UV index', value: '0' },
  { label: 'Sun event', value: '1' },
  { label: 'World clock', value: '2' },
  { label: 'Battery', value: '3' },
  { label: 'Weather', value: '4' },
  { label: 'Temperature', value: '5' },
  { label: 'Rain chance', value: '6' },
  { label: 'Steps', value: '7' },
  { label: 'Heart rate', value: '8' }
];

module.exports = [
  {
    type: 'heading',
    defaultValue: 'Signal Deck',
    size: 1
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Display'
      },
      {
        type: 'toggle',
        messageKey: 'darkMode',
        label: 'Dark mode',
        defaultValue: false
      },
      {
        type: 'select',
        messageKey: 'timeMode',
        label: 'Time format',
        defaultValue: '0',
        options: [
          { label: 'System', value: '0' },
          { label: '12-hour', value: '1' },
          { label: '24-hour', value: '2' }
        ]
      },
      {
        type: 'select',
        messageKey: 'tempUnit',
        label: 'Temperature',
        defaultValue: '0',
        options: [
          { label: 'Celsius', value: '0' },
          { label: 'Fahrenheit', value: '1' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Telemetry'
      },
      {
        type: 'slider',
        messageKey: 'stepGoal',
        label: 'Step goal',
        defaultValue: 8000,
        min: 1000,
        max: 50000,
        step: 500
      },
      {
        type: 'slider',
        messageKey: 'userAge',
        label: 'User age',
        defaultValue: 30,
        min: 10,
        max: 100,
        step: 1
      },
      {
        type: 'select',
        messageKey: 'world',
        label: 'World clock',
        defaultValue: 'nz',
        options: [
          { label: 'New Zealand', value: 'nz' },
          { label: 'UTC', value: 'utc' },
          { label: 'London', value: 'lon' },
          { label: 'New York', value: 'nyc' },
          { label: 'Los Angeles', value: 'la' },
          { label: 'Tokyo', value: 'tky' },
          { label: 'Sydney', value: 'syd' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Accent Colors'
      },
      {
        type: 'color',
        messageKey: 'hrColor',
        label: 'Heart',
        defaultValue: 'ff7393'
      },
      {
        type: 'color',
        messageKey: 'stepsColor',
        label: 'Steps',
        defaultValue: '00b96d'
      },
      {
        type: 'color',
        messageKey: 'rainColor',
        label: 'Rain',
        defaultValue: '009bff'
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Footer'
      },
      {
        type: 'select',
        messageKey: 'footerLeft',
        label: 'Left cell',
        defaultValue: '0',
        options: FOOTER_OPTIONS
      },
      {
        type: 'select',
        messageKey: 'footerCenter',
        label: 'Center cell',
        defaultValue: '1',
        options: FOOTER_OPTIONS
      },
      {
        type: 'select',
        messageKey: 'footerRight',
        label: 'Right cell',
        defaultValue: '2',
        options: FOOTER_OPTIONS
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Data'
      },
      {
        type: 'toggle',
        messageKey: 'demoFallback',
        label: 'Use demo values when sensors are empty',
        defaultValue: false
      },
      {
        type: 'toggle',
        messageKey: 'showWeather',
        label: 'Show weather',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'showHealth',
        label: 'Show health gauges',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'showBatteryPercent',
        label: 'Show battery percentage',
        defaultValue: true
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save Settings'
  }
];
