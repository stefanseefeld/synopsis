void func()
{
  try
  {
    cout << bar;
  }
  catch (string foo)
  {
    cout << "Error: " << foo << endl;
  }
  catch (...)
  {
    cout << "Catchall";
  }
}
