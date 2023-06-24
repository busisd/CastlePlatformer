// https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
#ifndef CASTLE_PLATFORMER_UTIL
#error Do not include this file directly, include util.h instead
#endif

namespace util
{
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

    bool Collides(Rect rect1, Rect rect2)
    {
        return (rect1.x + rect1.w) > rect2.x && (rect2.x + rect2.w) > rect1.x &&
               (rect1.y + rect1.h) > rect2.y && (rect2.y + rect2.h) > rect1.y;
    }

    template <typename T>
    void prettyLog(T t)
    {
        std::cout << t << std::endl;
    }

    template <typename T, typename... Args>
    void prettyLog(T t, Args... args)
    {
        std::cout << t << " ";
        prettyLog(args...);
    }

    int PositiveModulo(int i, int n)
    {
        return (i % n + n) % n;
    }
}
