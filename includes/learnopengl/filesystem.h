#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <unistd.h>
#include <limits.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

class FileSystem
{
public:
  static std::string getPath(const std::string &path)
  {
    static std::string basePath = findBasePath();
    return basePath + "/" + path;
  }

  // Get the directory where the executable is located
  static std::string getExecutableDir()
  {
    std::string exePath;

#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    exePath = buffer;
#elif defined(__APPLE__)
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
    {
      char realPath[PATH_MAX];
      if (realpath(buffer, realPath) != nullptr)
      {
        exePath = realPath;
      }
      else
      {
        exePath = buffer;
      }
    }
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
      buffer[len] = '\0';
      exePath = buffer;
    }
#endif

    // Remove the executable name to get the directory
    size_t lastSlash = exePath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
      return exePath.substr(0, lastSlash);
    }
    return ".";
  }

private:
  static bool fileExists(const std::string &path)
  {
    std::ifstream f(path);
    return f.good();
  }

  static bool hasResources(const std::string &path)
  {
    // Check if the resources directory exists with expected subdirectories
    return fileExists(path + "/resources/textures/awesomeface.png") ||
           fileExists(path + "/resources/objects/backpack/backpack.obj") ||
           fileExists(path + "/resources/images/place_holder.png");
  }

  static std::string findBasePath()
  {
    // First, check environment variable (for development override)
    const char *envRoot = std::getenv("LOGL_ROOT_PATH");
    if (envRoot != nullptr && hasResources(envRoot))
    {
      return std::string(envRoot);
    }

    // Get the executable directory
    std::string exeDir = getExecutableDir();

    // Check relative paths from executable location
    // This supports both development and distribution scenarios

    // Case 1: Resources are in the same directory as executable (distribution build)
    if (hasResources(exeDir))
    {
      return exeDir;
    }

    // Case 2: Executable is in bin/mecha_fight/, resources are in project root
    // Go up two levels: bin/mecha_fight/ -> bin/ -> project_root/
    std::string projectRoot = exeDir + "/../..";
    if (hasResources(projectRoot))
    {
      return projectRoot;
    }

    // Case 3: Executable is in build/ directory, resources are in project root
    projectRoot = exeDir + "/..";
    if (hasResources(projectRoot))
    {
      return projectRoot;
    }

    // Case 4: Check current working directory
    char cwd[PATH_MAX];
#ifdef _WIN32
    if (_getcwd(cwd, sizeof(cwd)) != nullptr)
#else
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
#endif
    {
      std::string cwdStr = cwd;
      if (hasResources(cwdStr))
      {
        return cwdStr;
      }

      // Also try going up from cwd (in case running from build/)
      projectRoot = cwdStr + "/..";
      if (hasResources(projectRoot))
      {
        return projectRoot;
      }
    }

    // Last resort: use executable directory and hope resources are there
    return exeDir;
  }
};

#endif // FILESYSTEM_H
