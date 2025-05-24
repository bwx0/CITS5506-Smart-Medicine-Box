# CITS5506 The Internet of Things Project - Smart Medicine Box

This is the repo for CITS5506 The Internet of Things Project - Smart Medicine Box (Group 16).

Follow the following steps to reproduce the prototype.

## Software

- HomeAssistant 2025.5.2
- ESPHome 2025.4.1

## Set up Home Assistant Locally

Linux:

```bash
docker run -d --name HA --privileged --restart=unless-stopped -e TZ=Australia/Perth -v /run/dbus:/run/dbus:ro --network=host ghcr.io/home-assistant/home-assistant:2025.5.2
```

Windows with WSL:

```bash
docker run -d --name HA --privileged --restart=unless-stopped -e TZ=Australia/Perth -v /run/dbus:/run/dbus:ro -p 8123:8123 ghcr.io/home-assistant/home-assistant:2025.5.2
```

Visit `http://localhost:8123`

## Set up Device Builder

Device Builder inside a Docker container does not have access to USB ports, thus we start it as a native app via Python.

```bash
pip install tornado esptool
# run device builder
esphome dashboard config
```

Visit `http://localhost:6052`

requirements.txt

```
esptool==4.8.1
pystick==0.1.6
```

## Configure UI

### Restore from Backup Files

You can set up the entire HA dashboard by recovering from backup files.

- Go to `Settings > System > Backups`
- Click three dots on the top right corner
- Upload the backup file (`HA_image/HA_backup_MQTT.tar`)
- Click on the uploaded file and restore

### Configure Manually 

Alternatively, you can follow the steps in [configureHA.md](./configureHA.md) to build the dashboard.

## Set up ESPHome

- Go to `./ESPHome_config`
- `medibox-mqtt.yaml` is the ESPHome config file to be compiled and flashed to our ESP32S3.
- Place `loadcell.h` and `dis.png` under the same directory as the config file, e.g. in `YOUR_ESPHOME_PROJECT_DIRECTORY/config/`.

## Set up MQTT Broker

We will use Eclipse Mosquitto as the MQTT broker.

### Create working directories

You’ll need three folders to persist configuration and data:

```bash
mkdir -p mosquitto/config
mkdir -p mosquitto/data
mkdir -p mosquitto/log
```

Create a basic config file at `mosquitto/config/mosquitto.conf`:

```
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log

allow_anonymous false
password_file /mosquitto/config/passwd

listener 1883
```

### Start the Docker container

Run this command to create a password file with your desired username:

```bash
docker run --rm -v "$PWD/mosquitto/config:/mosquitto/config" -it eclipse-mosquitto mosquitto_passwd -c /mosquitto/config/passwd cits5506
```

The username is `cits5506`. It will then prompt you for a password. Enter `cits5506passwd`.

Run the broker:

```bash
docker run -d --name mosquitto -p 1883:1883 -v "$PWD/mosquitto/config:/mosquitto/config" -v "$PWD/mosquitto/data:/mosquitto/data" -v "$PWD/mosquitto/log:/mosquitto/log" eclipse-mosquitto
```

### Integration

Connect ESPHome to the broker by adding the following lines to the config file:

```yaml
mqtt:
  broker: x.x.x.x  # IP of the machine running the Mosquitto container
  username: cits5506
  password: cits5506passwd
  discovery: true
```

