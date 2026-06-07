def on_loaded():
    log("example_client.py loaded")


def on_redis_message(channel, message):
    log(f"{channel}: {message}")
    publish(f"{channel}.processed", message)
    print("sent")


# Available API from Redisc:
# publish(channel: str, message: str)
# subscribe(channel: str)
# unsubscribe(channel: str)
# log(message: str)
