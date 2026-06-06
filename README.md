# Redisc

Redisc is a small Qt 6 Redis pub/sub client.

## Features

- Direct Redis login with host, port, username, password, and database.
- SSH tunnel login using `ssh -L` with host, port, user, identity file, and local port.
- Redis `PUBSUB CHANNELS` browser.
- Per-channel MDI windows for subscribed channels.
- Publish messages from each channel window.
- System, light, dark, and solarized color schemes.

## Build

```sh
cmake -S . -B build
cmake --build build
./build/redisc
```

The SSH tunnel mode requires the system `ssh` command to be available.
