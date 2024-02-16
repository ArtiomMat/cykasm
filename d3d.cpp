#include <vector>
#include <string>
#include <sstream>

template <typename t>
class mark
{
  protected:
  t* ptr;

  public:
  mark() : mark(1) 
  {}

  mark(int n)
  {
    ptr = new t[n];
  }

  t& operator[](int i)
  {
    
  }

  ~mark()
  {
    delete[] ptr;
  }
};

namespace d3d
{
  class frame_t
  {
    friend class draw_t;

    public:
    char* pixels = nullptr;

    frame_t(int width, int height, frame_t* parent);
    frame_t(int width, int height);

    void add_filter();
  };
  
  class thing_t
  {

  };

  /**
   * Virtual class meant for intheritance.
   * A filter takes in images and outputs 
  */
  class filter_t
  {
    public:
    
    /**
     * Not meant to modify pixels, rather to return a pixel value for each x and y.
    */
    virtual char run(frame_t& frame, int x, int y) = 0;
  };

  /**
   * Virtual class meant for intheritance.
   * A filter takes in images and outputs 
  */
  class plotter_t
  {
    public:
    
    /**
     * 
    */
    virtual void run(frame_t& frame) = 0;
  };
}
