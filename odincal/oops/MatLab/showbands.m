function showbands(S)
  f = skyfreq(S)/1.0e9;
  [m,n] = size(f);
  d = reshape(S.data,m,n);
  i = reshape([1:m*n]',m,n);
  t = sprintf('%08X  %s  %s  %s  %.2f sec', ...
	      S.stw, S.frontend, S.backend, S.type, S.inttime);
  subplot(2,1,1)
  plot(f,d)
  grid
  title(t)
  xlabel('frequency [GHz]')

  subplot(2,1,2)
  plot(i,d)
  grid
  title('channel order')
  