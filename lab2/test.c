int print(char *str, int n);

int main() {
  char str[] = "aa\n";
  str[3] = 0;
  print(&str[0], 111);
}