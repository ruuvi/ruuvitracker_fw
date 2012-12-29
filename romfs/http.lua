require('gsm')

function http_get(url)
	local data = ""
	--Start HTTP request
	gsm_cmd([[
AT+HTTPINIT
AT+HTTPPARA="CID","1"
AT+HTTPPARA="URL","]]..url..[["
AT+HTTPPARA="REDIR","1"]])
	gsm_send('AT+HTTPACTION=0')
	ret = gsm_wait("^%+HTTPACTION")
	local code
	local len
	_,_,code,len = string.find(ret,"%+HTTPACTION:0,(%d+),(%d+)")
	if code == "200" then
		if gsm_verbose==1 then print("Reading "..len.." bytes from "..url) end
		gsm_send('AT+HTTPREAD')
		gsm_wait("^%+HTTPREAD:"..len)
		--All bytes are data after this
		repeat
			local buf = uart.read(2,len)
			len = len-buf:len()
			data = data .. buf
		until len == 0
		--All bytes read, Wait for OK
		gsm_wait("^OK")
	else
		print("Error "..code)
	end
	--Terminate HTTP commands
	gsm_cmd("AT+HTTPTERM")
	return data
end

gsm_verbose = 0
print(http_get("http://st.dev.frontside.fi/"))

