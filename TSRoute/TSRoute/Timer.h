class Timer
{
public:
	Timer(void);
	
	int Load(void);
	int Unload(void);
	int Begin(void);
	int End(double& duration);
	int Wait(double tempo);
private:
	LARGE_INTEGER m_begin;
	LONGLONG      m_frequency;

};