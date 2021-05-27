#include <Arduino.h>
#include <painlessMesh.h>

static painlessMesh gMesh;

static void onNewConnection(uint32_t id)
{
  printf("Got new connection, id 0x%08x\n", id);
  gMesh.sendSingle(id, "Ahoj novej!");
}

static void onChangedConnections()
{
  printf("Topology change: %s\n", gMesh.subConnectionJson(true).c_str());
}

static void onReceive(uint32_t from, String &msg)
{
  printf("From 0x%08x: %s\n", from, msg.c_str());
}

void setup()
{
  // put your setup code here, to run once:
  gMesh.init("MojeSuperMesh", "HromadyRobotu");

  gMesh.onNewConnection(onNewConnection);
  gMesh.onChangedConnections(onChangedConnections);
  gMesh.onReceive(onReceive);
}

void loop()
{
  gMesh.update();
}
