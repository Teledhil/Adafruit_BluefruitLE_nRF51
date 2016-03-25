/**************************************************************************/
/*!
    @file     Adafruit_ATParser.cpp
    @author   hathach

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2016, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include "Adafruit_ATParser.h"

static inline char digit2ascii(uint8_t digit)
{
  return ( digit + ((digit) < 10 ? '0' : 'A') );
}

/******************************************************************************/
/*!
    @brief Constructor
*/
/******************************************************************************/
Adafruit_ATParser::Adafruit_ATParser(void)
{
  _mode    = BLUEFRUIT_MODE_COMMAND;
  _verbose = false;
}

/******************************************************************************/
/*!
    @brief  Convert buffer data to Byte Array String format such as 11-22-33-44
    e.g 0x11223344 --> 11-22-33-44

    @return number of character converted including dash '-'
*/
/******************************************************************************/
uint8_t Adafruit_ATParser::byteArray2String(char *str, const uint8_t* buffer, uint8_t count)
{
  for(uint8_t i=0; i<count; i++)
  {
    uint8_t byte = *buffer++;
    *str++ = digit2ascii((byte & 0xF0) >> 4);
    *str++ = digit2ascii(byte & 0x0F);

    if (i != count-1) *str++ = '-';
  }

  return (count*3) - 1;
}

/******************************************************************************/
/*!
    @brief  Read the whole response and check if it ended up with OK.
    @return true if response is ended with "OK". Otherwise it could be "ERROR"
*/
/******************************************************************************/
bool Adafruit_ATParser::waitForOK(void)
{
  if (_verbose) SerialDebug.print( F("\n<- ") );

  while ( readline() ) {
    //SerialDebug.println(buffer);
    if ( strcmp(buffer, "OK") == 0 ) return true;
    if ( strcmp(buffer, "ERROR") == 0 ) return false;
  }
  return false;
}

/******************************************************************************/
/*!
    @brief
    @param
*/
/******************************************************************************/
bool Adafruit_ATParser::send_arg_get_resp(int32_t* reply, uint8_t argcount, uint16_t argtype[], const void* args[])
{
  // Command arguments according to its type
  for(uint8_t i=0; i<argcount; i++)
  {
    // print '=' for WRITE mode
    if (i==0) print('=');

    switch (argtype[i] & 0xFF00)
    {
      case AT_ARGTYPE_STRING:

      break;

      case AT_ARGTYPE_BYTEARRAY:
      {
        uint8_t count        = lowByte(argtype[i]);
        uint8_t const * data = (uint8_t const*) args[i];

        while(count--)
        {
          uint8_t byte = *data++;
          write( digit2ascii((byte & 0xF0) >> 4) );
          write( digit2ascii(byte & 0x0F) );
          if ( count!=0 ) write('-');
        }
      }
      break;

      case AT_ARGTYPE_INT32:
        print( (int32_t) args[i] );
      break;
    }

    if (i != argcount-1) print(',');
  }
  println(); // execute command

  // parse integer response if required
  if (reply)
  {
    if (_verbose) SerialDebug.print( F("\n<- ") );
    (*reply) = readline_parseInt();
  }

  // check OK or ERROR status
  return waitForOK();
}

/******************************************************************************/
/*!
    @brief
    @param
*/
/******************************************************************************/
bool Adafruit_ATParser::atcommand_full(const char cmd[], int32_t* reply, uint8_t argcount, uint16_t argtype[], const void* args[])
{
  bool result;
  uint8_t current_mode = _mode;

  // switch mode if necessary to execute command
  if ( current_mode == BLUEFRUIT_MODE_DATA ) setMode(BLUEFRUIT_MODE_COMMAND);

  // Execute command with parameter and get response
  print(cmd);
  result = this->send_arg_get_resp(reply, argcount, argtype, args);

  // switch back if necessary
  if ( current_mode == BLUEFRUIT_MODE_DATA ) setMode(BLUEFRUIT_MODE_DATA);

  return result;
}

/******************************************************************************/
/*!
    @brief
    @param
*/
/******************************************************************************/
bool Adafruit_ATParser::atcommand_full(const __FlashStringHelper *cmd, int32_t* reply, uint8_t argcount, uint16_t argtype[], const void* args[])
{
  bool result;
  uint8_t current_mode = _mode;

  // switch mode if necessary to execute command
  if ( current_mode == BLUEFRUIT_MODE_DATA ) setMode(BLUEFRUIT_MODE_COMMAND);

  // Execute command with parameter and get response
  print(cmd);
  result = this->send_arg_get_resp(reply, argcount, argtype, args);

  // switch back if necessary
  if ( current_mode == BLUEFRUIT_MODE_DATA ) setMode(BLUEFRUIT_MODE_DATA);

  return result;
}

/******************************************************************************/
/*!
    @brief  Get a line of response data (see \ref readline) and try to interpret
            it to an integer number. If the number is prefix with '0x', it will
            be interpreted as hex number. This function also drop the rest of
            data to the end of the line.
*/
/******************************************************************************/
int32_t Adafruit_ATParser::readline_parseInt(void)
{
  uint16_t len = readline();
  if (len == 0) return 0;

  // also parsed hex number e.g 0xADAF
  int32_t val = strtol(buffer, NULL, 0);

  return val;
}

/******************************************************************************/
/*!
    @brief  Get a line of response data into provided buffer.

    @param[in] buf
               Provided buffer
    @param[in] bufsize
               buffer size
*/
/******************************************************************************/
uint16_t Adafruit_ATParser::readline(char * buf, uint16_t bufsize)
{
  uint16_t len = bufsize;
  uint16_t rd  = 0;

  do
  {
    rd = readline();

    uint16_t n = min(len, rd);
    memcpy(buf, buffer, n);

    buf += n;
    len -= n;
  } while ( (len > 0) && (rd == BLE_BUFSIZE) );

//  buf[bufsize - len] = 0; // null terminator

  return bufsize - len;
}

/******************************************************************************/
/*!
    @brief  Get (multiple) lines of response data into internal buffer.

    @param[in] timeout
               Timeout for each read() operation
    @param[in] multiline
               Read multiple lines

    @return    The number of bytes read. Data is available in the member .buffer.
               Note if the returned number is equal to BLE_BUFSIZE, the internal
               buffer is full before reaching endline. User should continue to
               call this function a few more times.
*/
/******************************************************************************/
uint16_t Adafruit_ATParser::readline(uint16_t timeout, boolean multiline)
{
  uint16_t replyidx = 0;

  while (timeout--) {
    while(available()) {
      char c =  read();
      //SerialDebug.println(c);
      if (c == '\r') continue;

      if (c == '\n') {
        if (replyidx == 0)   // the first '\n' is ignored
          continue;

        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      buffer[replyidx] = c;
      replyidx++;

      // Buffer is full
      if (replyidx >= BLE_BUFSIZE) {
        //if (_verbose) { SerialDebug.println("*overflow*"); }  // for my debuggin' only!
        timeout = 0;
        break;
      }
    }

    if (timeout == 0) break;
    delay(1);
  }
  buffer[replyidx] = 0;  // null term

  // Print out if is verbose
  if (_verbose && replyidx > 0)
  {
    SerialDebug.print(buffer);
    if (replyidx < BLE_BUFSIZE) SerialDebug.println();
  }

  return replyidx;
}

/******************************************************************************/
/*!
    @brief  Get raw binary data to internal buffer, only stop when encountering
            either "OK\r\n" or "ERROR\r\n" or timed out. Buffer does not contain
            OK or ERROR

    @param[in] timeout
               Timeout for each read() operation

    @return    The number of bytes read excluding OK, ERROR ending.
*/
/******************************************************************************/
uint16_t Adafruit_ATParser::readraw(uint16_t timeout)
{
  uint16_t replyidx = 0;

  while (timeout--) {
    while(available()) {
      char c =  read();

      if (c == '\n')
      {
        // done if ends with "OK\r\n"
        if ( (replyidx >= 3) && !strncmp(this->buffer + replyidx-3, "OK\r", 3) )
        {
          replyidx -= 3; // chop OK\r
          timeout = 0;
          break;
        }
        // done if ends with "ERROR\r\n"
        else if ((replyidx >= 6) && !strncmp(this->buffer + replyidx-6, "ERROR\r", 6))
        {
          replyidx -= 6; // chop ERROR\r
          timeout = 0;
          break;
        }
      }

      this->buffer[replyidx] = c;
      replyidx++;

      // Buffer is full
      if (replyidx >= BLE_BUFSIZE) {
        //if (_verbose) { SerialDebug.println("*overflow*"); }  // for my debuggin' only!
        timeout = 0;
        break;
      }
    }

    if (timeout == 0) break;
    delay(1);
  }
  this->buffer[replyidx] = 0;  // null term

  // Print out if is verbose
//  if (_verbose && replyidx > 0)
//  {
//    SerialDebug.print(buffer);
//    if (replyidx < BLE_BUFSIZE) SerialDebug.println();
//  }

  return replyidx;
}
