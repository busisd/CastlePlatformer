// https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
#ifndef CASTLE_PLATFORMER_UTIL
# error Do not include this file directly, include example.hpp instead
#endif

template <typename T>
void PrintSet(std::unordered_set<T> const &s)
{
    std::cout << "set{ ";
    for (const auto &elem : s)
    {
        std::cout << elem << " ";
    }
    std::cout << "}\n";
};

template <typename T>
bool Contains(std::unordered_set<T> const &s, T t)
{
    return s.find(t) != s.end();
}
