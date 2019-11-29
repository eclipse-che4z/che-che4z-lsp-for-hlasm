#include <filesystem>


int main()
{
    std::filesystem::path p("/home/");
    p.stem();
    return 0;
}
