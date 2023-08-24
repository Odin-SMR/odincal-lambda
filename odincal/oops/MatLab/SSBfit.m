% orbit number and order of baseline polynomial to fit
% orbit = 14317;
orbit = 14318;
order = 2;

% read spectra and table of commanded integration times
S = findspec('/var/www/odin/SSBtest/AOS555');
% S = findspec('/tmp/AOS572');
f = [S.skyfreq]';
o = [S.orbit]';
load /var/www/odin/SSBtest/ssbtest.mat -ascii

% distinguish lower and upper side-band measurements
% i550 = find(f < 555e9);
% i558 = find(f > 555e9);
fc = mean(f);
i550 = find(f < fc);
i558 = find(f > fc);

o1 = o(i550);
o2 = o(i558);
S1 = S(i550);
S2 = S(i558);

% loop through lower side-band spectra
% and fit baselines to channels [1:800] and [1000:1728]
% integrate intensity over channels [800:1000]
disp('lower side-band');
figure(1)
clf
n = length(S1);
m1 = zeros(n,1);
for i = 1:n
  f = frequency(S1(i));
  fn = -1.0+2.0*(f-min(f))/(max(f)-min(f));
  x = [fn(1:800);fn(1000:1728)];
  y = [S1(i).data(1:800); S1(i).data(1000:1728)];  
  c = polyfit(x,y,order);
  z = polyval(c,fn);
  y = S1(i).data-z;
  m1(i) = sum(y(800:1000));
  % plot(f,S1(i).data,'b-',f,z,'r-')
  plot(f,y,'b-'), grid
  a = axis;
  axis([a(1) a(2) -5 50])
  % waitforbuttonpress
  drawnow
end

% loop through upper side-band spectra
% and fit baselines to channels [1:800] and [1000:1728]
% integrate intensity over channels [800:1000]
disp('upper side-band');
clf
n = length(S2);
m2 = zeros(n,1);
for i = 1:n
  f = frequency(S2(i));
  fn = -1.0+2.0*(f-min(f))/(max(f)-min(f));
  x = [fn(1:800);fn(1000:1728)];
  y = [S2(i).data(1:800); S2(i).data(1000:1728)];  
  c = polyfit(x,y,order);
  z = polyval(c,fn);
  y = S2(i).data-z;
  m2(i) = sum(y(800:1000));
  plot(f,y,'b-'), grid
  a = axis;
  axis([a(1) a(2) -5 50])
  % waitforbuttonpress
  drawnow
end

% plot integrated intensities as function of orbit
figure(2)
subplot(2,1,1)
plot(o1-orbit,m1,'bo',o2-orbit,m2,'go')
grid
% axis([0.4 0.9 -200 800])

h = subplot(2,1,2);
o = ssbtest(:,1);
tcmd = ssbtest(:,3);
plot(o-orbit, tcmd, '+');
icmd = [25:2:55];
axis([0.4 0.9 min(icmd)/16.0 max(icmd)/16.0])
set(h,'YTickLabelMode','manual');
set(h,'YTick',icmd./16.0)
set(h,'YTickLabel', strcat('0x',dec2hex(icmd)))
xl = sprintf('orbit number - %d', orbit);
xlabel(xl)
ylabel('AC1 cmd.int.time (hex)')
grid
