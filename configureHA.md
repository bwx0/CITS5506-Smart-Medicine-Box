# Manually Set up Home Assistant

## Copy UI Config Files

- Copy `./medbox` to your HA configuration folder e.g. as `/config/medbox`.
- Override the configuration file (e.g. `/config/configuration.yaml`) with `./configuration_mqtt_ver.yaml`

## Add ESPHome Integration

### Add Device

- Go to `Settings > Devices & Services > Devices > Add Device > ESPHome`
- Enter the ESPHome device's IP address (assume it's connected to the same WIFI)
- Enter the encryption key found in the ESPHome device's config file
- Submit

### Enable HA Event

- Go to `Settings > Devices & Services > Integrations > ESPHome`
- Click `Configure`
- Turn on `Allow the device to perform Home Assistant actions`
- Submit

## Take Control over Dashboard

- HomeAssistant Home Page
- Click `Edit Dashboard` on the top right corner
- Click three dots
- Click `Take Control`

## Configuration

Paste at the end of `configuration.yaml` (located in `/config`).

```yaml
input_button : !include_dir_merge_named medbox/button
input_number: !include_dir_merge_named medbox/number
input_select: !include_dir_merge_named medbox/select
input_datetime: !include_dir_merge_named medbox/datetime
input_boolean: !include_dir_merge_named medbox/boolean
template: !include_dir_merge_list medbox/template
counter: !include_dir_merge_named medbox/counter
recorder:
  exclude:
    entities:
      - sensor.medibox_medibox_raw_weight
      - sensor.medibox_medibox_current_weight_window_avg
```

Please reload the files in the Developer tools panel each time you update a config.

**All the following files starting with `medbox/` should be placed in `/config/medbox`.**

## Pill Weighing Station

### Card

Create a card in the dashboard:

- Click `+ ADD CARD`
- Scroll to the bottom and click `Manual`
- Replace the config with the following:

```yaml
type: entities
title: Pill Weight Calibration
entities:
  - entity: sensor.guide
    icon: mdi:lightbulb
  - entity: input_button.pill_measure
    name: Measure
  - entity: sensor.medibox_medibox_raw_weight
    name: Box Weight
  - entity: input_number.pill_count
    name: Number of Pills
    icon: mdi:pill-multiple
  - entity: sensor.displayed_initial_weight
    name: Empty Box Weight
    icon: mdi:weight
  - entity: sensor.displayed_final_weight
    name: Box with Pills Weight
    icon: mdi:weight
  - entity: sensor.displayed_weight_per_pill
    name: Weight Per Pill
    icon: mdi:pill
```

### Automation

- Go to `Settings - Automations&Scenes`

- Click `Create Automation`
- Click `Create new automation`
- Click three dots
- Edit in YAML
- Replace the config with the following:

```yaml
alias: Pill weight measurement state machine
description: >
  Use an input_select as state, check scale-stabilized sensor, record A then B,
  compute per-pill weight, reset.
triggers:
  - trigger: state
    entity_id:
      - input_button.pill_measure
actions:
  - choose:
      - conditions:
          - condition: state
            entity_id: input_select.pill_scale_state
            state: Not measuring
        sequence:
          - data:
              entity_id: input_select.pill_scale_state
              option: Measuring initial weight
            action: input_select.select_option
          - data:
              title: Pill-scale
              message: >
                Place the empty box, wait for stability, then press Measure
                again.
            action: persistent_notification.create
            enabled: false
      - conditions:
          - condition: state
            entity_id: input_select.pill_scale_state
            state: Measuring initial weight
        sequence:
          - choose:
              - conditions:
                  - condition: state
                    entity_id: binary_sensor.medibox_medibox_weight_stabilized
                    state: "off"
                sequence:
                  - data:
                      title: Pill-scale
                      message: Scale unstable. Wait a moment and press Measure again.
                    action: persistent_notification.create
                    enabled: false
              - conditions:
                  - condition: state
                    entity_id: binary_sensor.medibox_medibox_weight_stabilized
                    state: "on"
                sequence:
                  - data:
                      entity_id: input_number.weight_a
                      value: "{{ states('sensor.medibox_medibox_raw_weight') }}"
                    action: input_number.set_value
                  - data:
                      entity_id: input_select.pill_scale_state
                      option: Measuring final weight
                    action: input_select.select_option
                  - data:
                      title: Pill-scale
                      message: >
                        Initial weight saved ({{ states('input_number.weight_a')
                        }} g).   Now add pills, wait for stability, then press
                        Measure again.
                    action: persistent_notification.create
                    enabled: false
                  - action: esphome.medibox_call_set_empty_weight
                    metadata: {}
                    data: {}
      - conditions:
          - condition: state
            entity_id: input_select.pill_scale_state
            state: Measuring final weight
        sequence:
          - choose:
              - conditions:
                  - condition: state
                    entity_id: binary_sensor.medibox_medibox_weight_stabilized
                    state: "off"
                sequence:
                  - data:
                      title: Pill-scale
                      message: Scale unstable. Wait a moment and press Measure again.
                    action: persistent_notification.create
                    enabled: false
              - conditions:
                  - condition: state
                    entity_id: binary_sensor.medibox_medibox_weight_stabilized
                    state: "on"
                sequence:
                  - data:
                      entity_id: input_number.weight_b
                      value: "{{ states('sensor.medibox_medibox_raw_weight') }}"
                    action: input_number.set_value
                  - data:
                      entity_id: input_number.weight_per_pill
                      value: |-
                        {{ (
                           (states('input_number.weight_b')|float
                            - states('input_number.weight_a')|float)
                           / states('input_number.pill_count')|float
                         ) | abs | round(3) }}
                    action: input_number.set_value
                  - data:
                      title: Pill-scale
                      message: >-
                        Per-pill weight = {{
                        states('input_number.weight_per_pill') }} g
                    action: persistent_notification.create
                    enabled: false
                  - data:
                      entity_id: input_select.pill_scale_state
                      option: Not measuring
                    action: input_select.select_option
                  - data:
                      entity_id: input_number.weight_a
                      value: 0
                    action: input_number.set_value
                  - data:
                      entity_id: input_number.weight_b
                      value: 0
                    action: input_number.set_value
    default:
      - data:
          title: Pill-scale
          message: Unexpected state—resetting measurement process.
        action: persistent_notification.create
        enabled: false
      - data:
          entity_id: input_select.pill_scale_state
          option: Not measuring
        action: input_select.select_option

```

### Reload the HomeAssistant

- Go to Developer Tools panel
- Click `Check Configuration` to validate the configs
- Click `ALL YAML CONFIGURATION` to reload

## Dashboard Panel

**Card**

Similarly, create a new card in the dashboard:

```yaml
type: entities
entities:
  - entity: sensor.medibox_status
    name: Box Status
    icon: mdi:package-variant-closed
  - entity: sensor.medibox_pills_taken
    name: Pills Taken
    icon: mdi:pill
  - entity: sensor.medibox_remaining_pills
    name: Remaining Pills
    icon: mdi:pill-multiple
title: Smart Medcine Box
```



## Analytics

### Medication Event Counting

Card:

```yaml
type: entities
title: Medication Summary
show_header_toggle: false
entities:
  - entity: sensor.medicine_total_doses
    name: Total doses recorded
    icon: mdi:pill
  - entity: counter.medicine_on_time
    name: Doses on time
    icon: mdi:calendar-check-outline
  - entity: counter.medicine_missed
    name: Missed doses
    icon: mdi:calendar-remove-outline
  - entity: counter.medicine_outside_window
    name: Doses outside of schedule
    icon: mdi:clock-alert-outline
  - entity: sensor.medicine_on_time_rate
    name: On-time rate (%)
    icon: mdi:percent-outline
```

Card  (chart):

```yaml
type: vertical-stack
cards:
  - chart_type: bar
    title: Dosage Time
    period: hour
    type: statistics-graph
    entities:
      - sensor.medicine_total_doses
    stat_types:
      - change
  - chart_type: line
    period: 5minute
    type: statistics-graph
    title: Medication Stats (Last 14 Days)
    entities:
      - sensor.medibox_on_time_count_tracker
      - sensor.medibox_missed_count_tracker
      - sensor.medibox_off_schd_count_tracker
    days_to_show: 4
    stat_types:
      - state
    hide_legend: false
    min_y_axis: 0
```

Automation:

```yaml
alias: Count medication events
description: ""
triggers:
  - event_type: esphome.medicine_event
    trigger: event
actions:
  - target:
      entity_id: counter.medicine_total
    action: counter.increment
    data: {}
  - choose:
      - conditions:
          - condition: template
            value_template: "{{ trigger.event.data.status == 'on_time' }}"
        sequence:
          - target:
              entity_id: counter.medicine_on_time
            action: counter.increment
            data: {}
      - conditions:
          - condition: template
            value_template: "{{ trigger.event.data.status == 'missed' }}"
        sequence:
          - target:
              entity_id: counter.medicine_missed
            action: counter.increment
            data: {}
      - conditions:
          - condition: template
            value_template: "{{ trigger.event.data.status == 'outside_window' }}"
        sequence:
          - target:
              entity_id: counter.medicine_outside_window
            action: counter.increment
            data: {}
    default:
      - action: notify.persistent_notification
        metadata: {}
        data:
          message: |
            Received unknown medication event: "{{ trigger.event.data.status }}"
mode: queued
```

## Medication History

Card:

```yaml
type: logbook
title: Recent Medication History (7 days)
target:
  entity_id:
    - input_boolean.medibox_logbook_dummy_on_time
    - input_boolean.medibox_logbook_dummy_missed
    - input_boolean.medibox_logbook_dummy_off_sched
    - input_boolean.medibox_logbook_dummy_refill
    - input_boolean.medibox_logbook_dummy_cancel
hours_to_show: 168
```



### Medication Event Reset

Automation:

```yaml
alias: Reset medication counters
description: ""
triggers:
  - entity_id: input_button.reset_medicine_counters
    trigger: state
actions:
  - target:
      entity_id:
        - counter.medicine_on_time
        - counter.medicine_missed
        - counter.medicine_outside_window
    action: counter.reset
    data: {}
```

Card:

```yaml
type: entities
title: Medication Summary
entities:
  - entity: input_button.reset_medicine_counters
    name: Reset Medication Counters
```



## Medibox Debugging

### Sensor Information

Card:

```yaml
type: entities
entities:
  - entity: sensor.medibox_medibox_current_weight_window_avg
    name: Windowed Weight
    icon: mdi:weight
  - entity: sensor.medibox_medibox_raw_weight
    name: Raw Weight
  - entity: sensor.medibox_medibox_stable_weight
    icon: mdi:scale
    name: Last Stabilised Weight
  - entity: binary_sensor.medibox_medibox_weight_stabilized
    name: Weight Stabilised
  - entity: binary_sensor.medibox_medibox_open
    name: Box Opened
  - entity: sensor.medibox_magnetic_strength
    name: Magnetic Strength
    icon: mdi:magnet
  - entity: sensor.medibox_pills_taken
    name: Pills Taken
    icon: mdi:pill-multiple
  - entity: sensor.medibox_remaining_pills
title: Smart Medcine Box
```

### Debugging Controls

Card:

```yaml
type: entities
title: Medibox Controls
entities:
  - type: call-service
    name: Set Zero
    icon: mdi:target
    action_name: Run
    service: esphome.medibox_set_zero
  - type: call-service
    name: Reset Total Pills Taken
    icon: mdi:target
    action_name: Run
    service: esphome.medibox_call_reset_total_pills_taken
  - type: call-service
    name: Set Empty Weight
    icon: mdi:target
    action_name: Run
    service: esphome.medibox_call_set_empty_weight
  - type: call-service
    name: Start Reminder
    icon: mdi:alarm-light
    action_name: Run
    service: esphome.medibox_call_start_reminder
  - type: call-service
    name: Stop Reminder
    icon: mdi:alarm-light-off
    action_name: Run
    service: esphome.medibox_call_stop_reminder
```

