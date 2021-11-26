#include <iostream>

#include <glar/application.h>

int main()
{
  try
  {
    glar::Application app;
    app.Run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
