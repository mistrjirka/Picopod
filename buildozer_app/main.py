import logging

# Set up logging
logging.basicConfig(level=logging.DEBUG, 
                   format='%(asctime)s - %(levelname)s - %(message)s',
                   filename='chat_app.log')
import asyncio
import traceback
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.textinput import TextInput
from kivy.uix.button import Button
from kivy.clock import Clock
from bleak import BleakClient, BleakScanner


WATCH_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
MSG_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
NEIGHCOUNT_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a9"

class ChatApp(App):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        logging.info("Initializing ChatApp")
        self.client = None
        self.connected = False

    def build(self):
        try:
            logging.info("Building UI")
            self.layout = BoxLayout(orientation='vertical')
            
            # Status label to show connection state
            self.status_label = Label(
                text='Status: Disconnected',
                size_hint=(1, 0.1)
            )
            
            # Chat history
            self.chat_history = Label(
                text='Welcome to LoRa Chat!\n',
                size_hint=(1, 0.7),
                text_size=(None, None),
                halign='left',
                valign='top'
            )
            
            # Message input
            self.message_input = TextInput(
                size_hint=(1, 0.1),
                multiline=False
            )
            
            # Send button
            self.send_button = Button(
                text='Send',
                size_hint=(1, 0.1)
            )
            self.send_button.bind(on_press=self.send_message)

            # Add widgets to layout
            self.layout.add_widget(self.status_label)
            self.layout.add_widget(self.chat_history)
            self.layout.add_widget(self.message_input)
            self.layout.add_widget(self.send_button)

            Clock.schedule_once(lambda dt: self.connect_to_watch())
            return self.layout

        except Exception as e:
            logging.error(f"Error in build: {str(e)}")
            logging.error(traceback.format_exc())
            # Create a simple error display if build fails
            layout = BoxLayout()
            layout.add_widget(Label(text=f'Error: {str(e)}'))
            return layout

    def send_message(self, instance):
        try:
            if self.connected:
                message = self.message_input.text
                asyncio.run(self.write_message(message))
                self.message_input.text = ''
                logging.info(f"Message sent: {message}")
        except Exception as e:
            logging.error(f"Error sending message: {str(e)}")
            logging.error(traceback.format_exc())

    async def write_message(self, message):
        await self.client.write_gatt_char(MSG_CHAR_UUID, message.encode())
        self.chat_history.text += f'\nMe: {message}'

    async def notification_handler(self, sender, data):
        message = data.decode()
        self.chat_history.text += f'\nWatch: {message}'

    async def connect(self):
        devices = await BleakScanner.discover()
        for d in devices:
            if d.name == 'LoraWatch':
                self.client = BleakClient(d.address)
                try:
                    await self.client.connect()
                    self.connected = True
                    await self.client.start_notify(MSG_CHAR_UUID, self.notification_handler)
                except Exception as e:
                    logging.error(f"Failed to connect: {e}")
                    logging.error(traceback.format_exc())
                break

    def connect_to_watch(self):
        asyncio.create_task(self.connect())

if __name__ == '__main__':
    try:
        logging.info("Starting ChatApp")
        ChatApp().run()
    except Exception as e:
        logging.error(f"Fatal error: {str(e)}")
        logging.error(traceback.format_exc())
