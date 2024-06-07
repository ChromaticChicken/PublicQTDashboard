import tkinter as tk
from tkinter import messagebox, font
from datetime import datetime, timedelta
import base64
from selenium import webdriver
from selenium.common.exceptions import NoSuchWindowException
from urllib.parse import unquote
import json
import webbrowser
import asyncio
import aiohttp
from aiohttp import BasicAuth
from filelock import FileLock

CONFIG_LOCATION: str = "ebay.config.json"

class MyDashboard:

	def __init__(self) -> None:

		self.root = tk.Tk()
		self.root.title("Refresh Token")

		self.root.geometry("800x400")

		default_font = font.nametofont("TkDefaultFont")
		default_font.configure(size=16)

		
		self.load_configuration()
		

		expiring_date = self.config['eBay']['refresh_token_expires_at']
		label = tk.Label(self.root, text=f"Are you sure you want to sign in? This only needs to take place every 1.5 years. \nTry refreshing the access token first\nThe next time this needs to be done is {expiring_date}")
		label.pack()

		button2 = tk.Button(self.root, text="Continue to sign in", pady=5, command=self.continue_sign_in)
		button2.pack()

		self.root.mainloop()


		
############################################################################
#                              Basic Functions                             #
############################################################################

	

	def load_configuration(self):
		with FileLock(CONFIG_LOCATION + ".lock"):
			with open(CONFIG_LOCATION, 'r') as file:
				self.config = json.load(file)

	def save_configuration(self):
		with FileLock(CONFIG_LOCATION + ".lock"):
			with open(CONFIG_LOCATION, 'w') as config_file:
				json.dump(self.config, config_file, indent=4)
	

	async def make_call(self, request_type: str, url: str, headers: dict, data: str = None, auth: tuple = None):
		async with aiohttp.ClientSession() as session:
			try:
				if request_type == "GET":
					if auth != None:
						async with session.get(url=url, headers=headers, auth=BasicAuth(*auth), timeout=10) as response:
							return await response.json(), response.status
					else:
						async with session.get(url=url, headers=headers, timeout=10) as response:
							return await response.json(), response.status
				elif request_type == "POST":
					if auth != None:
						async with session.post(url=url, headers=headers, data=data, auth=BasicAuth(*auth), timeout=10) as response:
							return await response.json(), response.status
					else:
						async with session.post(url=url, headers=headers, data=data, timeout=10) as response:
							return await response.json(), response.status
				elif request_type == "PUT":
					if auth != None:
						async with session.put(url=url, headers=headers, data=data, auth=BasicAuth(*auth), timeout=10) as response:
							return await response.json(), response.status
					else:
						async with session.put(url=url, headers=headers, data=data, timeout=10) as response:
							return await response.json(), response.status
					
			except aiohttp.client_exceptions.ContentTypeError:
				# this is here because the eBay api sometimes returns a xml file instead of a json file
				return await response.text(), response.status
					
			except asyncio.TimeoutError:
				messagebox.showerror("Timeout", "Request timed out")
				return None


############################################################################
#                          Access Token Functions                          #
############################################################################

	def continue_sign_in(self) -> None:
		for widget in self.root.pack_slaves():
			widget.destroy()

		label = tk.Label(self.root, text="What brower should be used?")
		label.pack()

		button1 = tk.Button(self.root, text="Chrome", pady=5, command=lambda: self.get_access_token('Chrome'))
		button1.pack()

		button2 = tk.Button(self.root, text="Firefox", pady=5, command=lambda: self.get_access_token('Firefox'))
		button2.pack()

		button3 = tk.Button(self.root, text="Edge", pady=5, command=lambda: self.get_access_token('Edge'))
		button3.pack()

		button4 = tk.Button(self.root, text="Other\nUse only as last resort", pady=5, command=self.open_custom_dialog)
		button4.pack()

	def open_custom_dialog(self) -> None:
		self.custum_dialog3 = tk.Toplevel(self.root)
		self.custum_dialog3.title("Sign in")

		label = tk.Label(self.custum_dialog3, text="Once signed in: copy the url, paste url into textbox below, then hit submit")
		label.pack()

		button1 = tk.Button(self.custum_dialog3, text="Open Default Webbrowser", pady=5, command=lambda: webbrowser.open(self.config['eBay']['sign_in_URL']))
		button1.pack()

		textbox = tk.Entry(self.custum_dialog3, width=40)
		textbox.pack()

		button2 = button1 = tk.Button(self.custum_dialog3, text="Submit", pady=5, command=lambda: self.submit(textbox.get()))
		button2.pack()

	def submit(self, url: str) -> None:
		# See this website for full documentation
		# https://developer.ebay.com/api-docs/static/oauth-refresh-token-request.html

		credentials = f"{self.config['eBay']['client_ID']}:{self.config['eBay']['client_secret']}"
		b64_credentials = base64.b64encode(bytes(credentials, 'utf-8')).decode('utf-8')

		url = f"https://api.ebay.com/identity/v1/oauth2/token"
		data = {
			'grant_type': "authorization_code",
			'code': "",
			'redirect_uri': self.config['eBay']['ru_name']
		}
		headers = {
			"Content-Type": "application/x-www-form-urlencoded",
			"Authorization": f"Basic {b64_credentials}"
		}

		start_index = url.find("code=") + len("code=")
		end_index = url.find("&expires")
		authorization_code = unquote(url[start_index:end_index])
		data["code"] = authorization_code

		response, status_code = asyncio.run(self.make_call("POST", url, headers, data))

		if status_code == 200:
			json_data = response.json()
			access_token: str = json_data.get('access_token', 'FAILED')
			refresh_token: str = json_data.get('refresh_token', 'FAILED')
			if access_token != 'FAILED':
				self.config['eBay']['access_token'] = access_token
			if refresh_token != 'FAILED':
				self.config['eBay']['refresh_token'] = refresh_token
				secs = json_data.get('refresh_token_expires_in', 0)
				self.config['eBay']['refresh_token_expires_at'] = (datetime.now() + timedelta(seconds=secs)).strftime("%a %b %d %Y")


			self.save_configuration()
			messagebox.showinfo(message='SUCCESS!!!')
		else:
			messagebox.showinfo("ERROR", f"FAILED\nStatus Code: {response.status_code}\n{response.text}")

	def get_access_token(self, webtype: str) -> None:
		# See this website for full documentation
		# https://developer.ebay.com/api-docs/static/oauth-refresh-token-request.html

		try:
			if webtype == 'Chrome':
				driver = webdriver.Chrome()
			elif webtype == 'Firefox':
				driver = webdriver.Firefox()
			elif webtype == 'Edge':
				driver == webdriver.Edge()
		except Exception as e:
			messagebox.showerror("ERROR", f"FAILED Try another browser type\n{e}")
			return

		driver.get(self.config['eBay']['sign_in_URL'])

		credentials = f"{self.config['eBay']['client_ID']}:{self.config['eBay']['client_secret']}"
		b64_credentials = base64.b64encode(bytes(credentials, 'utf-8')).decode('utf-8')

		url = f"https://api.ebay.com/identity/v1/oauth2/token"
		data = {
			'grant_type': "authorization_code",
			'code': "",
			'redirect_uri': self.config['eBay']['ru_name']
		}
		headers = {
			"Content-Type": "application/x-www-form-urlencoded",
			"Authorization": f"Basic {b64_credentials}"
		}
		
		have_code = False
		while not have_code:
			try:
				current_url = driver.current_url
			except NoSuchWindowException as e:
				messagebox.showerror("ERROR", f"WINDOW CLOSED TOO EARLY {e}")
				return
				
			if "code=" in current_url:
				start_index = current_url.find("code=") + len("code=")
				end_index = current_url.find("&expires")
				authorization_code = unquote(current_url[start_index:end_index])
				data["code"] = authorization_code
				driver.quit()

				response, status_code = asyncio.run(self.make_call("POST", url, headers, data))

				have_code = True
				
		if status_code == 200:
			json_data = response
			access_token: str = json_data.get('access_token', 'FAILED')
			refresh_token: str = json_data.get('refresh_token', 'FAILED')
			if access_token != 'FAILED':
				self.config['eBay']['access_token'] = access_token
			if refresh_token != 'FAILED':
				self.config['eBay']['refresh_token'] = refresh_token
				secs = json_data.get('refresh_token_expires_in', 0)
				self.config['eBay']['refresh_token_expires_at'] = (datetime.now() + timedelta(seconds=secs)).strftime("%a %b %d %Y")

			self.save_configuration()

			if webtype == 'Default':
				# if webtype == 'Default' then we arrived to here via the open_custom_dialog3 function and now we do want it to close because it worked
				self.custum_dialog3.destroy()
			messagebox.showinfo(message='SUCCESS!!!')
			self.root.destroy()
		else:
			messagebox.showinfo("ERROR", f"FAILED\nStatus Code: {status_code}\n{response}")



############################################################################
#                            ServiceNow Function                           #
############################################################################
  
	async def change_status(self, asset_tag: str, price: str = None, duplicate_asset_tags: list = None) -> None: 
		# This is called by the add_orders function as it is looping through the new orders

		# See this website for full documentation
		# https://docs.servicenow.com/bundle/utah-api-reference/page/integrate/inbound-rest/concept/c_TableAPI.html#title_table-POST
		
		# See this website for API Explorer
		# https://byusandbox.service-now.com/now/nav/ui/classic/params/target/%24restapi.do


		# Gets the sys_id of the asset_tag from ServiceNow, this is then used to perform other queries on the record
		url = f'https://byu.service-now.com/api/now/table/u_computer_surplus_items?sysparm_fields=sys_id&sysparm_limit=1&u_icn={asset_tag}'
        
		headers = {"Content-Type":"application/json","Accept":"application/json"}
        
		# gets the sys_id of the asset_tag from ServiceNow, this is then used to perform other queries on the record
		response_data, status_code = await self.make_call("GET", url, headers, auth=(self.config['serviceNow']['username'], self.config['serviceNow']['password'])) 
        
		if status_code != 200:
			messagebox.showerror("ERROR", f"FAILED\nStatus Code: {status_code}\n{response_data}")
			return None


		if len(response_data['result']) == 0:
			# if asset tag is not found then create a new record in ServiceNow
			if price == None and duplicate_asset_tags == None:
				# single item and no price
				data = '{' + f"\"u_icn\":\"{asset_tag}\",\"u_item_state\":\"Sold\"" + '}'
			elif price == None and duplicate_asset_tags != None:
				# single item with multiple asset tags and no price
				data = '{' + f"\"u_icn\":\"{asset_tag}\",\"u_item_state\":\"Sold\",\"u_description\":\"This item is part of a set{", ".join(duplicate_asset_tags)}\"" + '}'
			elif price != None and duplicate_asset_tags == None:
				# single item with a price
				data = '{' + f"\"u_icn\":\"{asset_tag}\",\"u_item_state\":\"Sold\",\"u_price\":" + "\"" + price + "\"}" + '}'
			else: #price != None and duplicate_asset_tags != None
				# single item with multiple asset tags with a price
				data = '{' + f"\"u_icn\":\"{asset_tag}\",\"u_item_state\":\"Sold\",\"u_price\":" + "\"" + price + f"\",\"u_description\":\"This item is part of a set{", ".join(duplicate_asset_tags)}\"" + '}'

			url = 'https://byu.service-now.com/api/now/table/u_computer_surplus_items'
			# make post request for new items
			response_data, status_code = await self.make_call("POST", url, headers, data=data, auth=(self.config['serviceNow']['username'], self.config['serviceNow']['password']))
		else:
			# if asset tag is found then edit record to have the correct status and price
			if price == None and duplicate_asset_tags == None:
				# single item and no price
				data = "{\"u_item_state\":\"Sold\"}"
			elif price == None and duplicate_asset_tags != None:
				# single item with multiple asset tags and no price
				data = '{' + f"\"u_item_state\":\"Sold\",\"u_description\":\"This item is part of a set{", ".join(duplicate_asset_tags)}\"" + '}'
			elif price != None and duplicate_asset_tags == None:
				# single item with a price
				data = "{\"u_item_state\":\"Sold\",\"u_price\":" + "\"" + price + "\"}"
			else: #price != None and duplicate_asset_tags != None
				# single item with multiple asset tags with a price
				data = '{' + f"\"u_item_state\":\"Sold\",\"u_price\":" + "\"" + price + f"\",\"u_description\":\"This item is part of a set{", ".join(duplicate_asset_tags)}\"" + '}'

			sys_id = response_data['result'][0]['sys_id']
			url = f'https://byu.service-now.com/api/now/table/u_computer_surplus_items/{sys_id}'

			# make put request for existing items
			response_data, status_code = await self.make_call("PUT", url, headers, data=data, auth=(self.config['serviceNow']['username'], self.config['serviceNow']['password']))

		if status_code != 200:
			messagebox.showerror("ERROR", f"FAILED\nStatus Code: {status_code}\n{response_data}")

############################################################################
#                               End of Class                               #
############################################################################
def main():
    app = MyDashboard()

if __name__ == "__main__":
    main()
