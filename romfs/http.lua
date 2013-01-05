require('common')
require('gsm')

module('http', package.seeall)

function get(url)
	local data = ""
	local user_agent = "RuuviTracker firmware/" .. firmware.version
	--Start HTTP request
	gsm.cmd([[
AT+HTTPINIT
AT+HTTPPARA="CID","1"
AT+HTTPPARA="URL","]]..url..[["
AT+HTTPPARA="UA","]]..user_agent..[["
AT+HTTPPARA="REDIR","1"]])
	gsm.send('AT+HTTPACTION=0')
	ret = gsm.wait("^%+HTTPACTION")
	local code
	local len
	_,_,code,len = string.find(ret,"%+HTTPACTION:0,(%d+),(%d+)")
	if code == "200" then
		if gsm.verbose==1 then print("Reading "..len.." bytes from "..url) end
		gsm.send('AT+HTTPREAD')
		gsm.wait("^%+HTTPREAD:"..len)
		--All bytes are data after this
		data = gsm.read_raw(len)
		--All bytes read, Wait for OK
		gsm.wait("^OK")
	else
		print("Error "..code)
	end
	--Terminate HTTP commands
	gsm.cmd("AT+HTTPTERM")
	return data
end

--URL Encode
--From http://lua-users.org/wiki/StringRecipes
function url_encode(str)
  if (str) then
    str = string.gsub (str, "\n", "\r\n")
    str = string.gsub (str, "([^%w ])",
        function (c) return string.format ("%%%02X", string.byte(c)) end)
    str = string.gsub (str, " ", "+")
  end
  return str	
end

function post(url, data, content)
	if content == nil then
		data = url_encode(data)
		content = 'application/x-www-form-urlencoded'
	end
	local user_agent = "RuuviTracker firmware/" .. firmware.version
	gsm.cmd([[
AT+HTTPINIT
AT+HTTPPARA="CID","1"
AT+HTTPPARA="URL","]]..url..[["
AT+HTTPPARA="UA","]]..user_agent..[["
AT+HTTPPARA="CONTENT","]]..content..[["
AT+HTTPPARA="REDIR","1"]])
	gsm.send('AT+HTTPDATA='..data:len()..',1000')
	gsm.wait("^DOWNLOAD")
	gsm.send(data)
	gsm.wait("^OK")
	gsm.send('AT+HTTPACTION=1')
	local ret = gsm.wait("^%+HTTPACTION")
	local code
	local len
	data = ""
	_,_,code,len = string.find(ret,"%+HTTPACTION:1,(%d+),(%d+)")
	if code == "200" then
		if gsm.verbose==1 then print("Reading "..len.." bytes from "..url) end
		gsm.send('AT+HTTPREAD')
		gsm.wait("^%+HTTPREAD:"..len)
		--All bytes are data after this
		data = gsm.read_raw(len)
		--All bytes read, Wait for OK
		gsm.wait("^OK")
	else
		print("Error "..code)
	end
	gsm.cmd("AT+HTTPTERM")
	return code,data
end

