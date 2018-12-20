#include <iostream>
#include <string>

template<typename T>
void swap(T& a, T& b)
{
    T c = std::move(a);
    a = std::move(b);
    b = std::move(c);
}

void reverse(std::string& str, int start, int end)
{
    while (start < end)
    {
        swap(str[start], str[end]);
        start++; end--;
    }
}

void reverse_word(std::string& str)
{
    int size = str.size();
    if (size <= 1) return;
    reverse(str, 0, size - 1);
    int fast = 0, slow = 0;
    while (slow < size)
    {
        if (str[slow] == ' ' && str[fast] == ' ') {
            slow++; fast++;
        } else if (fast == size || str[fast] == ' ') {
            fast--;
            reverse(str, slow, fast);
            slow = ++fast;
        } else {
            fast++;
        }
    }
}

int main()
{
    std::string str = "the sky is blue ";
    std::cout << "before reverse : " << str << std::endl;
    reverse_word(str);
    std::cout << "after reverse : " << str << std::endl;
}
