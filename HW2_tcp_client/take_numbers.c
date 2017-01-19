

int i = 0;
int result[20];
for(i = 0; i < 20; i++)
	result[i] = 0;

i = 0, l = 0;

while(a[i] != NULL)
{
	if( (a[i] >= '0') && (a[i] <= '9') )
	{
		result[l] = result[l] * 10 + (a[i] - '0');
	}
	if(a[i] == ',')
		l++;
}