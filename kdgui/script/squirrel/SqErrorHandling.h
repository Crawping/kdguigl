#ifndef SqErrorHandling_h
#define SqErrorHandling_h

extern int g_sq_error_result;
extern int g_sq_error_handling; // ������ʽ��0:��ʾ�����쳣��Ҳ�������ⲿ��1:�����ⲿ

bool isThrowSqException();

int 
#ifdef _MSC_VER
WINAPI 
#endif
KdPageError(void* arg, const SQChar* s, ...);

#endif // SqErrorHandling_h
