static void do_format_string(long value, char* f, char* o)
{
	int l = 0;

	/* parser */
	while(*f)
	{
		switch(*f)
		{
			case 'H':
			case 'M':
			case 's':
			case 'S':
			case 'F':
			case 'D':
				/* our char - increment length*/
				l++;
				/* check if future character is not the same */
				if((*f) != (*(f + 1)))
				{
					char local_format[128];
					char local_data[128];
					long local_value;

					/* create format string */
					sprintf(local_format, "%%%d.%dd", l, l);

					/* determinate value by format */
					switch(*f)
					{
						case 'H':
							local_value = (value)/1000/3600;
							break;
						case 'M':
							local_value = (value)/1000/60;
							local_value = local_value % 60;
							break;
						case 'S':
							local_value = (value)/1000;
							local_value = local_value % 60;
							break;
						case 's':
							local_value = (value)/1000;
							break;
						case 'F':
							local_value = (value % 1000) / 40;
							break;
						case 'D':
							switch(l)
							{
								case 1:
									local_value = (value % 1000) / 100;
									break;
								case 2:
									local_value = (value % 1000) / 10;
									break;
								default:
									local_value = (value % 1000);
									break;
							};
							break;
					};

					/* create new value */
					sprintf(local_data, local_format, local_value);

					/* append */
					strcat(o, local_data);
					o += strlen(local_data);

					/* reset counter */
					l = 0;
				};
				break;
			default:
				/* just append to main string */
				*o = *f; 
				o++;
				break;
		};
		/* next char */
		f++;
	};
	/* terminate string */
	*o = 0;
};
