#include <linux/input.h>
#include <linux/fb.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include <stdexcept>

class Fb {
 public:
  Fb() : m_fd(open("/dev/fb1", O_RDWR)) {
    if (m_fd == -1) {
      throw std::runtime_error(strerror(errno));
    }

    ioctl(m_fd, FBIOGET_VSCREENINFO, &m_vinfo);
    ioctl(m_fd, FBIOGET_FSCREENINFO, &m_finfo);
  }
  ~Fb() { close(m_fd); }

  int getFd() const { return m_fd; }

  int getScreenSize() const {
    return m_vinfo.yres_virtual * m_finfo.line_length;
  }

  int adjust(int& x, int& y) const {
    x = x % m_vinfo.xres;
    y = y % m_vinfo.yres;
  }

  int getLocation(int x, int y) const {
    return (x + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel / 8) +
           (y + m_vinfo.yoffset) * m_finfo.line_length;
  }

 private:
  int m_fd;
  fb_fix_screeninfo m_finfo;
  fb_var_screeninfo m_vinfo;
};

class Map {
 public:
  Map()
      : m_fb(),
        m_ptr(static_cast<char*>(mmap(nullptr, m_fb.getScreenSize(),
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED, m_fb.getFd(), 0))) {
    if (m_ptr == MAP_FAILED) {
      throw std::runtime_error(strerror(errno));
    }

    memset(m_ptr, 0, m_fb.getScreenSize());
  }
  ~Map() { munmap(m_ptr, m_fb.getScreenSize()); }

  int adjust(int& x, int& y) const { m_fb.adjust(x, y); }

  unsigned short& pixel(int x, int y) {
    return reinterpret_cast<unsigned short&>(
        m_ptr[m_fb.getLocation(x, y)]);
  }

 private:
  Fb m_fb;
  char* m_ptr;
};


int main(int argc, char* argv[]) {
  Map map;
  std::ifstream event("/dev/input/event0",
                      std::ifstream::in | std::ifstream::binary);
  input_event ev;
  int x(0), y(0);
  int xt(0), yt(0);
  unsigned short color(-1);

  map.pixel(x, y) = -1;

  while (event.read(reinterpret_cast<char*>(&ev), sizeof(ev))) {
    if (xt == x && yt == y) {
      do {
        xt = rand();
        yt = rand();

        map.adjust(xt, yt);
      } while (xt == x && yt == y);

      map.pixel(xt, yt) = 0xf000;
    }

    if (ev.type == EV_KEY && ev.value != 0) {
      map.pixel(x, y) = 0;

      switch (ev.code) {
        case KEY_UP:
          --y;
          break;
        case KEY_DOWN:
          ++y;
          break;
        case KEY_LEFT:
          --x;
          break;
        case KEY_RIGHT:
          ++x;
          break;
        case KEY_ENTER:
          color = rand();
          break;
      }

      map.adjust(x, y);
      map.pixel(x, y) = color;
    }
  }

  return 0;
}
