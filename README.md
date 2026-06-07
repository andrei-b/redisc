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
- Python client scripts via QtPyT, with Redis message callbacks and publish/subscribe API.
- Publish messages from each channel window.
- System, light, dark, dark color, and solarized color schemes.
- Active channel windows use a stronger title font and accent styling.
- Manageable named connection profiles with saved Redis fields, SSH tunnel fields, window geometry, and selected color scheme.

## Build

```sh
cmake -S . -B build
cmake --build build
./build/redisc
```

The SSH tunnel mode requires the system `ssh` command to be available.

## Python Client Scripts

Load a Python script from the Login tab's Python client section. When enabled,
Redisc calls this function for every arriving Redis message:

```py
def on_redis_message(channel, message):
    publish(f"{channel}.processed", message)
```

Available functions inside the script:

- `publish(channel, message)`
- `subscribe(channel)`
- `unsubscribe(channel)`
- `log(message)`

An optional `on_loaded()` function is called after the script is loaded.
See `scripts/example_client.py`.
