//#include <memory.h>

//#define assert(_E_) (void)(_assert_((_E_), #_E_, __FILE__, __LINE__) ? 0 : (*((char *)0)) = 0)
#define assert(_E_) (void)(_assert_((_E_), #_E_, __FILE__, __LINE__))
//#define assert(_E_)

#define halt (void)(_assert_(0, "Halt reached", __FILE__, __LINE__))

bool _assert_(int exp, const char *exp_text, const char *file, int line);

////////////////////////////////////////////////////////

inline int nblocks16(int n)
{
  return ((n + 15) / 16);
}

