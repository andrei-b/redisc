def on_loaded():
    log("example_client.py loaded")


def on_redis_message(channel, message):
    log(f"{channel}: {message}")

    # Example processor: echo JSON-ish messages to a derived channel.
    if message.strip().startswith("{"):
        publish(f"{channel}.processed", message)


# Available API from Redisc:
# publish(channel: str, message: str)
# subscribe(channel: str)
# unsubscribe(channel: str)
# log(message: str)
