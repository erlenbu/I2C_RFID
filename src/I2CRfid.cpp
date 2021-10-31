#include "I2CRfid.h"

/*********************  Master side *********************/

namespace I2CMaster
{

  int RFIDSlaveObject::getReaderAmount() {
    int reader_amount = makeRequest(I2C_Request::SENSOR_AMOUNT);
    Serial.print("getReaderAmount(): "); Serial.println(reader_amount);
    if(reader_amount != -1) 
    {
      m_Readers = reader_amount;
    }
    return reader_amount;   
  }

  int RFIDSlaveObject::getRFIDStatus() { 
    int status = -1;
    if(m_Readers > 0) {
      status = makeRequest(I2C_Request::STATUS, m_Readers*sizeof(byte));
    }
    return status;
  }

  int RFIDSlaveObject::makeRequest(I2C_Request request, int len) {
    int data;

    if(len <= 0) {
      Serial.print("ERROR makeRequest() invalid len value: "); Serial.println(len);
      return -1;
    }

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

    int bytes = Wire.requestFrom((int)m_SlaveNumber, len);
    //RFID status is 1 byte long
    if(len != bytes) {
      Serial.print("RFID status wrong length: "); Serial.println(bytes);
      return -1;
    }

    while (Wire.available()) {
      data = Wire.read();
      Serial.print(data);
      if(-1 == data) {
        Serial.println("ERROR: Wire::read() returned -1");
        return -1;
      }
    }
    Serial.println();

    return data;
  }


  I2CRfidMaster::I2CRfidMaster()
  : m_SlaveAmount(0), m_RfidHandler(RfidHandler())
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

  void I2CRfidMaster::addRfidReader(uint8_t ss_pin, uint8_t rst_pin, void(*callback)(int,TAG_STATUS), UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) {
    m_RfidHandler.addRfidReader(ss_pin, rst_pin, callback, companion_tag);

  }

  void I2CRfidMaster::clearCache() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.clearCache();
    }
  }

  void I2CRfidMaster::read() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.read();
    }
  }
}

/*********************  Slave side *********************/

namespace I2CSlave
{
  uint8_t I2CRfidSlave::m_Request = 0;
  uint8_t I2CRfidSlave::m_RfidState = 0;
  RfidHandler I2CRfidSlave::m_RfidHandler = RfidHandler();
  byte* I2CRfidSlave::m_States = nullptr;

  I2CRfidSlave::I2CRfidSlave(int slave_no) {
    Wire.begin(slave_no);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
    m_Request = I2C_Request::STATUS;
  }

  I2CRfidSlave::~I2CRfidSlave() {}

  void I2CRfidSlave::I2CWrite(byte* data, uint8_t length) {
    for(int byte = 0; byte < length; byte++)
    {
      if(sizeof(data[byte]) != Wire.write(data[byte])) {
        Serial.print("ERROR: I2CWrite() write failed: return value ");
        Serial.println(data[byte]);
      }
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
    uint8_t reader_amount = m_RfidHandler.getReaderAmount();
    if(m_Request == I2C_Request::STATUS) {
      I2CWrite(m_States, reader_amount);
    }
    else if(m_Request == I2C_Request::SENSOR_AMOUNT) {
      I2CWrite(&reader_amount, sizeof(reader_amount));
    }
    else if(m_Request == I2C_Request::CLEAR_UID_CACHE) {
      Serial.println("Clearing cache!");
      clearCache();
    }
    else {
      Serial.println("Unknown request");
    }
  }

  void I2CRfidSlave::tagChangeEvent(int id, TAG_STATUS state) {
    m_States[id] = state;
    Serial.print("Reader "); Serial.print(id); Serial.print(" state "); Serial.println(m_States[id]);
    for(int i = 0; i < m_RfidHandler.getReaderAmount(); i++)
    {
      Serial.print(m_States[i]);
    }
    Serial.println();

  }

  void I2CRfidSlave::addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) {
    m_RfidHandler.addRfidReader(ss_pin, rst_pin, tagChangeEvent, companion_tag);
    m_States = (byte*) realloc(m_States, m_RfidHandler.getReaderAmount() * sizeof(byte));
    for(int i = 0; i < m_RfidHandler.getReaderAmount(); i++) { m_States[i] = 0; }
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