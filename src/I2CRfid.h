#ifndef I2C_RFID_H
#define I2C_RFID_H

#include <WSWire.h>
#include <Arduino.h>
#include "RfidReader.h"

enum I2C_Request {
  SENSOR_AMOUNT = 0,
  STATUS = 1,
  CLEAR_UID_CACHE = 2
};

enum I2C_Tags_Status {
  TAGS_MISSING = 0,
  TAGS_INCORRECT = 1,
  TAGS_CORRECT = 2
};



/*********************  Master side *********************/


namespace I2CMaster 
{
  class RFIDSlaveObject
  {
  public:
    RFIDSlaveObject(uint8_t slave_no) : m_SlaveNumber(slave_no), m_ReaderAmount(0) { Serial.print("Creating RFIDSlaveObject "); Serial.println(slave_no);}
    ~RFIDSlaveObject() {}

    void InitSlave();
    
    uint8_t getReaderAmount() { return m_ReaderAmount; }
    inline int clearUidCache() { return makeRequest(I2C_Request::CLEAR_UID_CACHE);   }
    int readRfidState(byte* data) { return makeRequest(I2C_Request::STATUS, data, m_ReaderAmount*sizeof(byte)); }

  private:
    uint8_t m_SlaveNumber;
    uint8_t m_ReaderAmount; 
    TAG_STATUS m_RfidStatus;
    

    inline int readSlaveReaderAmount(byte& data) { return makeRequest(I2C_Request::SENSOR_AMOUNT, &data); }
    int makeRequest(I2C_Request request, byte* data_received = nullptr, int len = 1);
  };

  class I2CRfidMaster
  {
  public:
    I2CRfidMaster();
    ~I2CRfidMaster();

    void InitMaster();

    void addI2CSlave(uint8_t slave_no);

    int checkGlobalTagStatus();

    int getGlobalReaderAmount();

    I2C_Tags_Status checkTagStatus(uint8_t reader_amount, byte* data);

    void addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    void clearCache();

    void read();

  private:
    RFIDSlaveObject* m_SlaveArray;
    uint8_t* m_SlaveArrayMapping;
    uint8_t m_SlaveAmount;
    RfidHandler m_RfidHandler;
    I2C_Tags_Status m_GlobalStatus;

    uint8_t getSlaveArrayElement(uint8_t slave_no);

    int createGlobalReaderArray(byte* tag_status);
  };
}


/*********************  Slave side *********************/

namespace I2CSlave
{
  class I2CRfidSlave
  {
    public:
      I2CRfidSlave(int slave_no);

      ~I2CRfidSlave();

      void addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

      static void clearCache();

      void read();

    private:
      static uint8_t m_Request;
      static uint8_t m_RfidState;
      static RfidHandler m_RfidHandler;

      static void I2CWrite(byte* data, uint8_t length = 1);

      static void receiveEvent(int num_bytes);

      static void requestEvent();
  };
}
#endif