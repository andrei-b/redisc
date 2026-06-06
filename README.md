# Redisc

Redisc is a small Qt 6 Redis pub/sub client.

## Features

- Direct Redis login with host, port, username, password, and database.
- SSH tunnel login using `ssh -L` with host, port, user, identity file, and local port.
- Redis `PUBSUB CHANNELS` browser.
- Persistent recent channel list, merged with live Redis channel results.
- Per-channel MDI windows for subscribed channels.
- JSON messages can be copied into separate collapsible tree windows.
- Configurable persistent font and text color for channel windows.
- Publish messages from each channel window.
- System, light, dark, and solarized color schemes.
- Manageable named connection profiles with saved Redis fields, SSH tunnel fields, window geometry, and selected color scheme.

## Build

```sh
cmake -S . -B build
cmake --build build
./build/redisc
```

The SSH tunnel mode requires the system `ssh` command to be available.
