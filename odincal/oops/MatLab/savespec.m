function savespec(S, filename)
% SAVESPEC  Save a binary Odin spectrum
%    SAVESPEC(S, file) takes the MatLab struct S holding an 
%    Odin spectrum and writes it to disk to a file called 'file'.
%    See also LOADSPEC.
  
  fid = fopen(filename, 'w');
  if fid == -1
    warning('cant open file');
    disp(filename);
  else
    fwrite(fid, hex2dec(S.version), 'ushort');
    fwrite(fid, hex2dec(S.level), 'ushort');
    fwrite(fid, hex2dec(S.quality), 'ulong');

    fwrite(fid, S.stw, 'ulong');
    fwrite(fid, S.mjd, 'double');
    fwrite(fid, S.orbit, 'double');
    fwrite(fid, S.lst, 'float');

    fwrite(fid, S.source, 'char');
    dis = ['AERO';'ASTR'];
    i = strmatch(S.discipline, dis);
    if isempty(i), i = 0; end
    fwrite(fid, i, 'ushort');
      
    if strcmp(S.discipline,'ASTR')
      topic = ['SOLSYS';'STARS ';'EXTGAL';'LMC   ';'PRIMOL';
	       'SPECTR';'CHEM  ';'GPLANE';'GCENTR';'GMC   ';
	       'SFORM ';'DCLOUD';'SHOCKS';'PDR   ';'HILAT ';
	       'ABSORB';'ORION ';'CALOBS';'COMMIS'         ];
    else
      topic = ['STRAT ';'ODD_N ';'ODD_H ';'WATER ';'SUMMER' ];
    end
    i = strmatch(S.topic, topic);
    if isempty(i), i = 0; end
    fwrite(fid, i, 'ushort');

    fwrite(fid, S.spectrum, 'short');

    obsmode = ['TPW';'SSW';'LSW';'FSW'];
    i = strmatch(S.obsmode, obsmode);
    if isempty(i), i = 0; end
    fwrite(fid, i, 'ushort');
  
    type = ['SIG';'REF';'CAL';'CMB';'DRK';'SK1';'SK2';'SPE';'SSB';'AVE'];
    i = strmatch(S.type, type);
    if isempty(i), i = 0; end
    fwrite(fid, i, 'ushort');
  
    frontend = ['555';'495';'572';'549';'119';'SPL'];
    i = strmatch(S.frontend, frontend);
    if isempty(i), i = 0; end
    fwrite(fid, i, 'ushort');
  
    backend = ['AC1';'AC2';'AOS';'FBA'];
    i = strmatch(S.backend, backend);
    if isempty(i), i = 0; end
    fwrite(fid, i, 'ushort');
  
    fwrite(fid, hex2dec(S.skybeamhit), 'ushort');

    fwrite(fid, S.ra2000, 'float');
    fwrite(fid, S.dec2000, 'float');
    fwrite(fid, S.vsource, 'float');
    fwrite(fid, S.u, 'float');

    fwrite(fid, S.qtarget, 'double')';
    fwrite(fid, S.qachieved, 'double')';
    fwrite(fid, S.qerror, 'double')';
  
    fwrite(fid, S.gpspos, 'double')';
    fwrite(fid, S.gpsvel, 'double')';

    fwrite(fid, S.sunpos, 'double')';
    fwrite(fid, S.moonpos, 'double')';
    fwrite(fid, S.sunzd, 'float');

    fwrite(fid, S.vgeo, 'float');
    fwrite(fid, S.vlsr, 'float');

    fwrite(fid, S.tcal, 'float');
    fwrite(fid, S.tsys, 'float');
    fwrite(fid, S.trec, 'float');

    fwrite(fid, S.lofreq, 'double');
    fwrite(fid, S.skyfreq, 'double');
    fwrite(fid, S.restfreq, 'double');
    fwrite(fid, S.maxsup, 'double');
    fwrite(fid, S.freqthrow, 'double');
    fwrite(fid, S.freqres, 'double');
    fwrite(fid, S.freqcal, 'double')';

    fwrite(fid, S.intmode, 'int');
    fwrite(fid, S.inttime, 'float');
    fwrite(fid, S.efftime, 'float');

    fwrite(fid, S.channels, 'int');
    fwrite(fid, S.data, 'float');
 
    fclose(fid);
  end