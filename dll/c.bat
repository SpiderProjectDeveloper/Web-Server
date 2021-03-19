cl /Fe"serverweb.dll" /EHsc /LD /DLL /W2 /O2 /GL /Gy /std:c++17 server.cpp server_response.cpp helpers.cpp server_auth.cpp Ws2_32.lib Shlwapi.lib
