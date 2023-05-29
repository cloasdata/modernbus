#include <Arduino.h>

#include "mbparser.h"

#include "modernbus_server_response.h"

ServerResponse::ServerResponse(ModbusRequest *request)
:   _request{request}
{};
