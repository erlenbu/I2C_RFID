#include "I2CRfid.h"

// /*********************  Master side *********************/

namespace I2CMaster
{
  int RFIDSlaveObject::makeRequest(I2C_Request request) {
    int data;

    Wire.beginTransmission(m_SlaveNumber);

    size_t byte_size = Wire.write(request);
    if(1 != byte_size) {
      Serial.print("Wrote wrong byte amount to slave?"); Serial.println(byte_size);
      return -1;
    }
    uint8_t status = Wire.endTransmission();
    if(0 != status) {
      Serial.print("ERROR: Wire::endTransmission() returned status "); Serial.println(status);
      return -1;
    }

    int bytes = Wire.requestFrom((int)m_SlaveNumber, 1);
    //RFID status is 1 byte long
    if(1 != bytes) {
      Serial.print("RFID status wrong length: "); Serial.println(bytes);
      return -1;
    }

    while (Wire.available()) {
      data = Wire.read();
      if(-1 == data) {
        Serial.println("ERROR: Wire::read() returned -1");
        return -1;
      }
    }

    return data;
  }

  I2CRfidMaster::I2CRfidMaster()
  : m_SlaveAmount(0)
  {
    Wire.begin();
    m_SlaveArray = (RFIDSlaveObject*)malloc(sizeof(RFIDSlaveObject));
    m_SlaveArrayMapping = (uint8_t*)malloc(sizeof(uint8_t));
  }

  I2CRfidMaster::~I2CRfidMaster()
  {
    free(m_SlaveArray);
    free(m_SlaveArrayMapping);
    free(m_SlaveAmount);
  }

  void I2CRfidMaster::addI2CSlave(uint8_t slave_no)
  {
    m_SlaveAmount += 1;
    m_SlaveArray = (RFIDSlaveObject*)realloc(m_SlaveArray, m_SlaveAmount * sizeof(RFIDSlaveObject));
    m_SlaveArray[m_SlaveAmount-1] = RFIDSlaveObject(slave_no);
    m_SlaveArrayMapping = (uint8_t*)realloc(m_SlaveArrayMapping, m_SlaveAmount * sizeof(uint8_t));
    m_SlaveArrayMapping[m_SlaveAmount-1] = slave_no;
    Serial.print("Amount of slaves: "); Serial.println(m_SlaveAmount);
  }

  uint8_t I2CRfidMaster::getSlaveArrayElement(uint8_t slave_no)
  {
    int element = -1;
    for(uint8_t a = 0; a < m_SlaveAmount; a++) {
      if(m_SlaveArrayMapping[a] == slave_no)
        element = a;
    }

    if(element == -1) {
      Serial.print("Slave id "); Serial.print(slave_no); Serial.println(" is not known");
    }
    return element;
  }

  int I2CRfidMaster::getSlaveRFIDStatus(uint8_t slave_no)
  {
    int status = -1;
    int element = getSlaveArrayElement(slave_no);
    if(element != -1) {
      status = m_SlaveArray[element].getRFIDStatus();
    }
    else {
      Serial.println("Invalid slave number");
    }

    return status;
  }

  int I2CRfidMaster::getSlaveRFIDSensorAmount(uint8_t slave_no) {
    int element = getSlaveArrayElement(slave_no);
    int sensorAmount = -1;
    if(element != -1) {
      sensorAmount = m_SlaveArray[element].getReaderAmount();
    }
    else {
      Serial.println("Invalid slave number");
    }
    return sensorAmount;
  }

  void I2CRfidMaster::clearSlaveCache(uint8_t slave_no) {
    int element = getSlaveArrayElement(slave_no);
    if(element != -1) {
      m_SlaveArray[element].clearUidCache();
    }
    else {
      Serial.println("Invalid slave number");
    }
  }
}

/*********************  Slave side *********************/

namespace I2CSlave
{
  uint8_t I2CRfidSlave::m_Request = 0;
  uint8_t I2CRfidSlave::m_RfidState = 0;

  I2CRfidSlave::I2CRfidSlave(int slave_no) {
    Wire.begin(slave_no);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
    m_Request = I2C_Request::STATUS;
  }

  I2CRfidSlave::~I2CRfidSlave() {}

  void I2CRfidSlave::I2CWrite(uint8_t data) {
    if(sizeof(data) != Wire.write(data)) {
      Serial.print("ERROR: I2CWrite() write failed: return value ");
      Serial.println(data);
    }
  }

  void I2CRfidSlave::receiveEvent(int num_bytes) {
    if(num_bytes == 1) {
      m_Request = Wire.read();
    }
    else {
      Serial.println("Incorrect message size");
    }
  }

  void I2CRfidSlave::requestEvent() {
    if(m_Request == I2C_Request::STATUS) {
      I2CWrite(m_RfidState);
    }
    else if(m_Request == I2C_Request::SENSOR_AMOUNT) {
      I2CWrite(m_RfidHandler.getReaderAmount());
    }
    else if(m_Request == I2C_Request::CLEAR_UID_CACHE) {
      Serial.println("Clearing cache!");
      clearCache();
    }
    else {
      Serial.println("Unknown request");
    }
  }

  void I2CRfidSlave::tagChangeEvent(int id, bool state) {
      if(true == state) {
        bitSet(m_RfidState, id);
      }
      else {
        bitClear(m_RfidState, id);
      }
  }

  void I2CRfidSlave::addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) {
    m_RfidHandler.addRfidReader(ss_pin, rst_pin, tagChangeEvent, companion_tag);
  }

  void I2CRfidSlave::clearCache() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.clearCache();
    }
  }

  void I2CRfidSlave::read() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.read();
    }
  }
}