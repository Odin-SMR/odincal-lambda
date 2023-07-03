function [isig,ical,iref,isk1,isk2] = specindx(S)
% SPECINDX return indices of different spectrum types
%    [ISIG,ICAL,IREF,ISK1,ISK2] = SPECINDX(S) takes an array of Odin
%    spectra and will return the indeces of spectra of a given type in
%    vectors ISIG, ICAL,IREF, ISK1 and ISK2, corresponding to
%    spectra with extension of 'SIG', 'CAL', 'REF', 'SK1' and
%    'SK2', respectively.
%
%    [ISIG,ICAL,IREF] = SPECINDX(S) will return the indeces non-signal
%    spectra 'REF', 'SK1' and 'SK2' in one vector IREF.

  typ = strvcat(S.type);
  isig = strmatch('SIG',typ);
  ical = strmatch('CAL',typ);
  iref = strmatch('REF',typ);
  isk1 = strmatch('SK1',typ);
  isk2 = strmatch('SK2',typ);
  if nargout == 3
    iref = sort([iref;isk1;isk2]);
  end
