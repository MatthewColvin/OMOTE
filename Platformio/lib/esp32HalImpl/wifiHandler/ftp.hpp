class ftp {
  public:
  static void begin(const char *_user, const char *_pass);
  static void end();
  static void sync();
};