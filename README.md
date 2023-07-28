### LOG_KV
A cpp sync logger support logfmt. Header only, thread safe, very easy to use and easy to extend(to async log or other custom function).

depend on fmt(https://github.com/fmtlib/fmt.git), you need to install it

The logfmt format was first documented by Brandur Leach in this article(https://brandur.org/logfmt). 

using like this:
```C++
SET_LOG_FILE("./log/test.log");
SET_LOG_LEVEL(LOG_INFO);
std::string str = "test_str";
std::string_view str_view = "test_string_view";
const char *str_char_pointer = "test_char_pointer";
const char str_char_array[] = "test_char_array";
LOG_KV(LOG_INFO, ("key_1", str), ("key_2", std::string("str")),
        ("key_3", std::string_view("key3_str_view")), ("key_4", str_view),
        ("key_5", str_char_pointer), ("key_6", str_char_array));
LOG_KV(LOG_INFO, ("key_nan", NAN),
        ("key_inf", std::numeric_limits<double>::infinity()));
auto vec = std::vector<int>{1, 2, 3};
auto map = std::unordered_map<int, double>{{1, 2.1}, {2, 3.3}};
LOG_KV(LOG_INFO, ("key_vec", vec), ("key_map", map));
```
Then you can get output:
```
ts=2023-07-28T14:04:22.487900 tid=3000666 level=INFO  caller=test.cc:20 func=main key_1=1 key_2=1 key_3="m" 
ts=2023-07-28T14:04:22.487956 tid=3000666 level=INFO  caller=test.cc:25 func=main key_1="test_str" key_2="str" key_3="key3_str_view" key_4="test_string_view" key_5="test_char_pointer" key_6="test_char_array" 
ts=2023-07-28T14:04:22.487967 tid=3000666 level=INFO  caller=test.cc:28 func=main key_nan=nan key_inf=inf 
ts=2023-07-28T14:04:22.487982 tid=3000666 level=INFO  caller=test.cc:32 func=main key_vec=[1, 2, 3] key_map={2: 3.3, 1: 2.1} 
```
When you use it, you need to pay attention:
- key must be string literal which can be deal by macro.
- must using like this (key, value).
- (key, value) amount support 16, if you need more, you can extend by yourself.
