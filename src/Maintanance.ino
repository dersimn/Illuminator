Thread infoThread = Thread();

void setup_Maintanance() {
  infoThread.onRun([](){
    publishLight();
  });
  infoThread.setInterval(MAINTENANCE_INTERVAL);
  threadControl.add(&infoThread);
}
