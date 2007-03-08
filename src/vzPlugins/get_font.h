static HANDLE _fonts_list_lock;
static vzHash<vzTTFont*> vzTTFontList;
static vzTTFont* get_font(char* name, struct vzTTFontParams* params)
{
	// lock list
	WaitForSingleObject(_fonts_list_lock,INFINITE);

	/* build a hash name */
	long hash = 0;
	for(int i = 0; i < sizeof(struct vzTTFontParams); i++)
	{
		int b = (hash & 0x80000000)?1:0;
		hash <<= 1;
		hash |= b;
		hash ^= (unsigned long)(((unsigned char*)params)[i]);
	};

	// build key name is 
	char key[1024];
	sprintf(key,"%s/%.8X", name, hash);

	// try to find object
	vzTTFont* temp;
	if(temp = vzTTFontList.find(key))
	{
		ReleaseMutex(_fonts_list_lock);
		return temp;
	};
	
	// object not found - create and load
	temp = new vzTTFont(name, params);

	// check if object is ready - success font load
	if(!temp->ready())
	{
#ifdef _DEBUG
		printf("FAILED(not loaded)\n");
#endif

		delete temp;
		ReleaseMutex(_fonts_list_lock);
		return NULL;
	};

	// save object in hash
	vzTTFontList.push(key,temp);

	ReleaseMutex(_fonts_list_lock);

#ifdef _DEBUG
		printf("OK(added)\n");
#endif

	
	return temp;
};
