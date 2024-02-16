#include <string>

template <typename t>
class complex
{
  public:
  t x, y;

  complex()
  {
    x = 0;
    y = 0;
  }

  complex(t x)
  {
    this->x = x;
    y = 0;
  }
  complex(t x, t y)
  {
    this->x = x;
    this->y = y;
  }

  complex square()
  {
    return complex(x*x - y*y, 2*x*y);
  }

  complex operator+(complex other)
  {
    return complex(x+other.x, y+other.y);
  }
  complex operator+(t other)
  {
    return complex(x+other, y);
  }

  complex operator-(complex other)
  {
    return complex(x-other.x, y-other.y);
  }
  complex operator-(t other)
  {
    return complex(x-other, y);
  }

  complex operator*(complex other)
  {
    return complex(x*other.x - y*other.y, x*other.y + y*other.x);
  }
  complex operator*(t other)
  {
    return complex(x*other, y*other);
  }

  complex operator/(complex other)
  {
    t tmp = other.x*other.x + other.y*other.y;
    complex other_c = other.conjugate();
    return (*this * other_c)/tmp;
  }
  complex operator/(t other)
  {
    return complex(x/other, y/other);
  }

  complex conjugate()
  {
    return complex(x, -y);
  }

  t modulus()
  {
    return 
  }

  t argument()
  {

  }

 
};

typedef complex<int> cint;
typedef complex<float> cfloat;

template <typename t>
std::string to_string(complex<t> n)
{
  std::ostringstream oss;
  if (n.x == 0 && n.y == 0)
  {
    return "0";
  }
  
  if (n.x == 0)
  {
    oss << n.y;
    oss << 'i';
  }
  else if (n.y == 0)
  {
    oss << n.x;
  }
  else
  {
    oss << n.x;
    oss << '+';
    oss << n.y;
    oss << 'i';
  }
  return oss.str();
}

int main()
{
  cfloat x(3, 4), y(2, 2);
  cfloat z = (x/y);
  z = 1;
  printf("%s", to_string(z).c_str());
}
