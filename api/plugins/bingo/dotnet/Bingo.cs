﻿using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;

namespace com.ggasoftware.indigo
{
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
	public unsafe class Bingo
	{
		private Indigo _indigo;
		private BingoLib _bingo_lib;
		private int _bingo;

		public Bingo (Indigo indigo)
		{
			String dllpath = indigo.getDllPath ();
			string libraryName;
			IndigoDllLoader dll_loader = IndigoDllLoader.Instance;
			switch (Environment.OSVersion.Platform) 
			{
			case PlatformID.Win32NT:
				libraryName = "bingo.dll";
				dll_loader.loadLibrary (dllpath, libraryName, "com.ggasoftware.indigo.Properties.ResourcesWin", false);
				break;
			case PlatformID.Unix:
				if (IndigoDllLoader.isMac()) 
				{
					libraryName = "libbingo.dylib";
					dll_loader.loadLibrary (dllpath, libraryName, "com.ggasoftware.indigo.Properties.ResourcesMac", false);
				} 
				else 
				{
					libraryName = "libbingo.so";
					dll_loader.loadLibrary (dllpath, libraryName, "com.ggasoftware.indigo.Properties.ResourcesLinux", false);
				}
				break;
			default:
				throw new PlatformNotSupportedException (String.Format ("Unsupported platform: {0}", Environment.OSVersion.Platform));
			}
			
			_bingo_lib = dll_loader.getInterface<BingoLib>(libraryName);
			_indigo = indigo;
			_bingo = -1;
		}

		public void createDatabaseFile(string location, string type, string options)
		{
			_indigo.setSessionID();
			if (options == null) 
			{
				options = "";
			}
			_bingo = _indigo.checkResult(_bingo_lib.bingoCreateDatabaseFile(location, type, options));
		}

		public void loadDatabaseFile(string location, string type)
		{
			_indigo.setSessionID();
			_bingo = _indigo.checkResult(_bingo_lib.bingoLoadDatabaseFile(location, type));
		}

		public void deleteDatabase()
		{
			_indigo.setSessionID();
			_indigo.checkResult(_bingo_lib.bingoDeleteDatabase(_bingo));
			_bingo = -1;			                   
		}

		public void insert(BingoObject record) 
		{
			_indigo.setSessionID();
			_indigo.checkResult(_bingo_lib.bingoInsertRecordObj(_bingo, record.self));
		}

		public void delete(int index) 
		{
			_indigo.setSessionID();
			_indigo.checkResult(_bingo_lib.bingoDeleteRecord(_bingo, index));
		}

		public BingoObject searchSub(IndigoObject query, string options)
		{
			_indigo.setSessionID();
			if (options == null) 
			{
				options = "";
			}
			return new BingoObject(_indigo.checkResult(_bingo_lib.bingoSearchSub(_bingo, query.self, options)));
		}

		public BingoObject searchSim(IndigoObject query, float min, float max, string metric)
		{
			_indigo.setSessionID();
			if (metric == null) 
			{
				metric = "tanimoto";
			}
			return new BingoObject(_indigo.checkResult(_bingo_lib.bingoSearchSub(_bingo, query.self, min, max, metric)));
		}
	}
}
