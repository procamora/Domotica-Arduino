from telethon import TelegramClient, events, sync
import sys

# These example values won't work. You must get your own api_id and
# api_hash from https://my.telegram.org, under API Development.
api_id = 1529220
api_hash = 'ased356738ba58af8dsf5873dee663er35'

client = TelegramClient('session_name', api_id, api_hash)
client.start()

print(client.get_me().stringify())

client.send_message('@domotica_pablo_bot', sys.argv[1])
