def on_loaded():
    log("example_client.py loaded")


def on_redis_message(channel, message):
    if not channel.endswith(".processed"):
        publish(f"{channel}.processed", message)
   


# Available API from Redisc:
# publish(channel: str, message: str)
# subscribe(channel: str)
# unsubscribe(channel: str)
# log(message: str)
