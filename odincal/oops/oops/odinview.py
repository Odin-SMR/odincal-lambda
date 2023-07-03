import odin
from Tkinter import *

import odinfile
import Pmw
import os
import time
import string


class HeaderInfo:
    def __init__(self, frame, label, row):
        self.label = label
        Label(frame,
              text=string.capitalize(label + ':'),
              justify='right').grid(row=row, column=0, sticky=E)
        self.value = Label(frame, font='fixed',
                           width=16, background='white')
        self.value.grid(row=row, column=1, sticky=W)

    def setValue(self, value):
        self.value.configure(text=str(value))


def setInfo(info, field, value):
    for i in range(len(info)):
        if info[i].label == field:
            info[i].setValue(value)


class SpectrumViewer:
    def __init__(self, master):
        self.spectrum = odin.Spectrum()
        self.view = None
        self.selected = -1

        self.master = master
        # master.minsize(1000,600)
        master.minsize(800, 550)
        self.createMenu(master)

        if not Pmw.Blt.haveblt(master):
            print("BLT is not installed!")

        f = Frame(master)
        f.pack(side='top', expand=1, fill='both')
        f1 = Frame(f)
        f1.pack(side='left', fill='y')
        Label(f1, text='Files').pack(side='top', pady=5)

        f1 = Frame(f1)
        f1.pack(side='top', expand=1, fill='y', padx=10)
        self.lb = Listbox(f1, selectmode='single', width=16,
                          font='fixed', background='white')
        self.sb = Scrollbar(f1, command=self.lb.yview)
        self.lb.configure(yscrollcommand=self.sb.set)
        self.lb.pack(side='left', expand=1, fill='y')
        self.sb.pack(side='left', fill="y")
        self.lb.bind('<ButtonRelease-1>', self.select)
        self.lb.bind('<Key-Down>', self.next)
        self.lb.bind('<Key-Up>', self.prev)
        self.lb.bind('<Key-Delete>', self.delete)
        self.lb.focus_set()

        self.g = Pmw.Blt.Graph(f)
        self.g.pack(side='left', expand=1, fill='both')
        self.g.bind("<ButtonPress-1>", self.mouseDown)
        self.g.bind("<ButtonRelease-1>", self.mouseUp)

        x = [0] * 10
        y = [0] * 10
        for i in range(0, 10):
            x[i] = float(i)
            y[i] = 0.0

        self.g.pen_create("line",
                          color="blue",
                          symbol="")
        self.g.pen_create("empty",
                          color="blue",
                          symbol="circle",
                          fill="",
                          pixels=7,
                          linewidth=0)
        self.g.pen_create("filled",
                          color="blue",
                          symbol="circle",
                          fill="red",
                          pixels=7,
                          linewidth=0)
        self.g.line_create("One",
                           xdata=tuple(x),
                           ydata=tuple(y),
                           pen="line",
                           label="")
        self.g.line_create("Two",
                           xdata=(),
                           ydata=(),
                           label="")
        self.g.element_configure("Two", hide=1)
        self.g.element_bind("Two", "<ButtonPress-2>", self.mouseClicked)

        self.g.configure(title='Spectrum viewer')
        self.g.grid_on()
        self.showCrosshairs()

        f = Frame(f)
        f.pack(side='right', fill='y')
        g = Frame(f)
        g.pack(side='top', anchor=N, padx=10)

        Label(g, text='Header info').grid(row=0, columnspan=2, pady=5)
        self.info = []
        row = 1
        for field in ('stw', 'source', 'ra2000', 'dec2000', 'orbit', 'mjd',
                      'inttime', 'backend', 'frontend', 'quality', 'skyfreq'):
            self.info.append(HeaderInfo(g, field, row))
            row = row + 1

        g = Frame(f)
        g.pack(side='bottom', anchor=S, pady=10)
        Label(g, text='Cursor info').grid(row=0, columnspan=2, pady=5)
        self.xpos = Label(g, text='', font='fixed', width=12,
                          relief='sunken', background='white')
        self.ypos = Label(g, text='', font='fixed', width=10,
                          relief='sunken', background='white')
        self.xpos.grid(row=1, col=0)
        self.ypos.grid(row=1, col=1)
        f = Frame(master, relief='raised')
        f.pack(side='bottom', fill='x', pady=5)
        Button(f, text='Quit',
               command=master.quit).pack(side='right', padx=10)
        self.movie = Button(f, text='Movie', width=6,
                            command=self.animate)
        self.movie.pack(side='right', padx=10)

        # Label(f,text='Orbit:').pack(side='left')
        # self.orbit = Pmw.Counter(f,datatype='integer',
        #                          entryfield_value=532,
        #                          entry_width=5)
        # self.orbit.pack(side='left',padx=5)

        Button(f, text='Clear',
               command=self.clrSelect).pack(side='left', padx=5)

    def createMenu(self, master):
        self.menuBar = Pmw.MenuBar(master,
                                   hull_relief='raised', hull_borderwidth=1)
        self.menuBar.pack(fill='x')

        # File menu
        self.menuBar.addmenu('File', 'helptxt')
        self.menuBar.addmenuitem('File', 'command', '<help context>',
                                 label='Open', command=self.openFile)
        self.menuBar.addmenuitem('File', 'separator')
        self.menuBar.addmenuitem('File', 'command', '<help context>',
                                 label='Quit', command=master.quit)

        # View menu
        self.menuBar.addmenu('View', 'helptxt')
        self.menuBar.addmenuitem('View', 'command', '<help context>',
                                 label='Spectrum', command=self.viewSpec)
        self.menuBar.addmenuitem('View', 'command', '<help context>',
                                 label='Mean', command=self.viewMean)
        self.menuBar.addmenuitem('View', 'command', '<help context>',
                                 label='RA-Dec', command=self.viewPos)

        # Config menu
        self.menuBar.addmenu('Config', 'helptxt')
        self.menuBar.addmenuitem('Config', 'command', '<help context>',
                                 label='Y-axis', command=self.getYScale)
        self.menuBar.addmenuitem('Config', 'command', '<help context>',
                                 label='X-axis', command=self.getXScale)

        # Tools menu
        self.menuBar.addmenu('Tools', 'helptxt')
        self.menuBar.addmenuitem('Tools', 'command', '<help context>',
                                 label='Add', command=self.addSpectra)
        self.menuBar.addmenuitem('Tools', 'command', '<help context>',
                                 label='Calibrate', command=self.calSpectra)
        # self.menuBar.addmenuitem('Tools', 'separator')

    def viewSpec(self):
        if self.view == "spectrum":
            return

        self.g.element_configure("Two", hide=1)
        self.g.element_configure("One", hide=0)
        self.g.yaxis_configure(min="", max="", title="")
        self.g.xaxis_configure(min="", max="",
                               descending=0,
                               title="frequency [GHz]")
        self.view = "spectrum"
        sels = self.lb.curselection()
        if len(sels) > 0:
            i = int(sels[0])
        else:
            i = 0
        self.getEntry(i)

    def viewMean(self):
        self.view = "mean"
        n = self.lb.index('end')
        x = [0] * n
        y = [0] * n
        for i in range(n):
            entry = os.path.join(self.dir, self.lb.get(i))
            s = odin.Spectrum(entry)
            m = s.Moment()
            x[i] = s.orbit
            y[i] = m[0]
        self.x = x
        self.y = y
        self.g.yaxis_configure(min="", max="", title="mean intensity")
        self.g.xaxis_configure(min="", max="", descending=0, title="orbit")
        self.g.element_configure("One", hide=1)
        self.g.element_configure("Two", hide=0)
        self.g.element_configure("Two",
                                 xdata=tuple(x),
                                 ydata=tuple(y),
                                 pen="empty")
        self.getEntry(0)
        self.master.update()

    def viewPos(self):
        self.view = "position"
        n = self.lb.index('end')
        x = [0] * n
        y = [0] * n
        for i in range(n):
            entry = os.path.join(self.dir, self.lb.get(i))
            s = odin.Spectrum(entry)
            x[i] = s.ra2000
            y[i] = s.dec2000
        self.x = x
        self.y = y
        self.g.yaxis_configure(min="", max="", title="Dec J2000")
        self.g.xaxis_configure(min="", max="", descending=1, title="RA J2000")
        self.g.element_configure("One", hide=1)
        self.g.element_configure("Two", hide=0)
        self.g.element_configure("Two",
                                 xdata=tuple(x),
                                 ydata=tuple(y),
                                 pen="empty")
        self.getEntry(0)
        self.master.update()

    def getYScale(self):
        top = Toplevel(self.master)
        self.top = top
        f = Frame(top)
        f.pack(side='top', pady=10)
        Label(f, text='Fix y-axis scale',
              justify='left').grid(row=0, columnspan=2, sticky=W, pady=5)
        Label(f, text='min', justify='left').grid(row=1, column=0, sticky=W)
        Label(f, text='max', justify='left').grid(row=2, column=0, sticky=W)
        self.ymin = Entry(f, font='fixed', width=10, justify='center',
                          relief='sunken', background='white')
        self.ymin.grid(row=1, column=1, sticky=W)
        self.ymax = Entry(f, font='fixed', width=10, justify='center',
                          relief='sunken', background='white')
        self.ymax.grid(row=2, column=1, sticky=W)
        f = Frame(top)
        f.pack(side='bottom', fill='x')

        (ymin, ymax) = self.g.yaxis_limits()
        self.ymin.insert(0, str(ymin))
        self.ymax.insert(0, str(ymax))

        Button(f, text='Cancel', width=5,
               command=top.destroy).pack(side='left')
        Button(f, text='Ok', width=5,
               command=self.setYScale).pack(side='right')

    def setYScale(self):
        ymin = self.ymin.get()
        ymax = self.ymax.get()
        self.g.yaxis_configure(min=ymin, max=ymax)
        self.top.destroy()

    def getXScale(self):
        top = Toplevel(self.master)
        self.top = top
        f = Frame(top)
        f.pack(side='top', pady=10)
        Label(f, text='Fix x-axis scale',
              justify='left').grid(row=0, columnspan=2, sticky=W, pady=5)
        Label(f, text='min', justify='left').grid(row=1, column=0, sticky=W)
        Label(f, text='max', justify='left').grid(row=2, column=0, sticky=W)
        self.xmin = Entry(f, font='fixed', width=10, justify='center',
                          relief='sunken', background='white')
        self.xmin.grid(row=1, column=1, sticky=W)
        self.xmax = Entry(f, font='fixed', width=10, justify='center',
                          relief='sunken', background='white')
        self.xmax.grid(row=2, column=1, sticky=W)
        f = Frame(top)
        f.pack(side='bottom', fill='x')

        (xmin, xmax) = self.g.xaxis_limits()
        self.xmin.insert(0, str(xmin))
        self.xmax.insert(0, str(xmax))

        Button(f, text='Cancel', width=5,
               command=top.destroy).pack(side='left')
        Button(f, text='Ok', width=5,
               command=self.setXScale).pack(side='right')

    def setXScale(self):
        xmin = self.xmin.get()
        xmax = self.xmax.get()
        self.g.xaxis_configure(min=xmin, max=xmax)
        self.top.destroy()

    def zoom(self, x0, y0, x1, y1):
        self.g.xaxis_configure(min=x0, max=x1)
        self.g.yaxis_configure(min=y0, max=y1)

    def mouseDrag(self, ev):
        global x0, y0, x1, y1
        (x1, y1) = self.g.invtransform(ev.x, ev.y)

        self.g.marker_configure(
            "marking rectangle", coords=(
                x0, y0, x1, y0, x1, y1, x0, y1, x0, y0))

    def mouseClicked(self, ev):
        el = self.g.element_closest(ev.x, ev.y)
        i = el["index"]
        print "mouseClicked", i
        self.getEntry(i)

    def mouseUp(self, ev):
        global dragging
        global x0, y0, x1, y1
        if dragging:
            self.g.unbind(sequence="<Motion>")
            self.g.marker_delete("marking rectangle")
            if x0 != x1 and y0 != y1:
                # make sure the coordinates are sorted
                if x0 > x1:
                    x0, x1 = x1, x0
                if y0 > y1:
                    y0, y1 = y1, y0
                if ev.num == 1:
                    self.zoom(x0, y0, x1, y1)
                else:
                    (X0, X1) = self.g.xaxis_limits()
                    k = (X1 - X0) / (x1 - x0)
                    x0 = X0 - (x0 - X0) * k
                    x1 = X1 + (X1 - x1) * k
                    (Y0, Y1) = self.g.yaxis_limits()
                    k = (Y1 - Y0) / (y1 - y0)
                    y0 = Y0 - (y0 - Y0) * k
                    y1 = Y1 + (Y1 - y1) * k
                    self.zoom(x0, y0, x1, y1)
            self.showCrosshairs()

    def mouseDown(self, ev):
        global dragging, x0, y0
        dragging = 0
        if self.g.inside(ev.x, ev.y):
            self.showCrosshairs()
            dragging = 1
            (x0, y0) = self.g.invtransform(ev.x, ev.y)
            self.g.marker_create("line",
                                 name="marking rectangle",
                                 dashes=(2, 2))
            self.g.bind("<Motion>", self.mouseDrag)

    def mouseMove(self, ev):
        self.g.crosshairs_configure(position="@" + str(ev.x) + "," + str(ev.y))
        x, y = self.g.invtransform(ev.x, ev.y)
        self.xpos.configure(text="%9.4f" % (x))
        self.ypos.configure(text="%9.4f" % (y))

    def clrSelect(self):
        self.lb.delete(0, 'end')

    def getEntry(self, i):
        self.selected = i
        self.lb.selection_set(i)
        # self.lb.activate(i)
        self.lb.see(i)
        entry = os.path.join(self.dir, self.lb.get(i))
        s = odin.Spectrum(entry)
        self.spectrum = s
        if self.view == "spectrum":
            self.showSpectrum(s)
        else:
            self.fillHeader(s)
            self.w = [0.0] * len(self.x)
            self.w[i] = 1.0
            styles = (("empty", 0.0, 0.9), ("filled", 1.0, 1.9))
            self.g.element_configure("Two",
                                     styles=styles,
                                     xdata=tuple(self.x),
                                     ydata=tuple(self.y),
                                     weights=tuple(self.w))

    def select(self, ev):
        sels = self.lb.curselection()
        if len(sels) > 0:
            i = int(sels[0])
            self.getEntry(i)

    def delete(self, ev):
        sels = self.lb.curselection()
        if len(sels) > 0:
            i = int(sels[0])
            self.lb.selection_clear(i)
            entry = os.path.join(self.dir, self.lb.get(i))
            self.lb.delete(i)
            if self.view != "spectrum":
                del self.x[i]
                del self.y[i]
                self.g.element_configure("Two",
                                         xdata=tuple(self.x),
                                         ydata=tuple(self.y),
                                         pen="empty")
            print "delete", entry
            os.remove(entry)
            if i >= self.lb.index('end'):
                i = self.lb.index('end') - 1
            self.getEntry(i)

    def next(self, ev):
        sels = self.lb.curselection()
        if len(sels) > 0:
            i = int(sels[0])
            if i < self.lb.index('end') - 1:
                self.lb.selection_clear(0, 'end')
                i = i + 1
                self.getEntry(i)

    def prev(self, ev):
        sels = self.lb.curselection()
        if len(sels) > 0:
            i = int(sels[0])
            if i > 0:
                self.lb.selection_clear(0, 'end')
                i = i - 1
                self.getEntry(i)

    def showCrosshairs(self):
        hide = not int(self.g.crosshairs_cget('hide'))
        self.g.crosshairs_configure(hide=hide, dashes="1")
        if(hide):
            self.g.unbind('<Motion>')
        else:
            self.g.bind('<Motion>', self.mouseMove)

    def animate(self):
        if self.movie.cget('text') == 'Movie':
            self.movie.configure(text='Stop')
            self.g.configure(bufferelements=0)
            for i in range(self.lb.index('end')):
                self.lb.selection_clear(0, 'end')
                self.getEntry(i)
                self.master.update_idletasks()
                self.master.update()
                if self.movie.cget('text') == 'Movie':
                    break
            self.movie.configure(text='Movie')
        else:
            self.movie.configure(text='Movie')

    def stop(self):
        self.stopmovie = 1

    def insertFile(self, file):
        self.lb.insert('end', os.path.split(file)[-1])

    def openFile(self):
        files = odinfile.selectScans(self.master)
        if len(files) > 0:
            self.dir = os.path.split(files[0])[0]
            print self.dir
            files.sort()
            for fname in files:
                self.insertFile(fname)
            self.viewSpec()
            # self.getEntry(0)

    def fillHeader(self, s):
        info = self.info
        setInfo(info, 'stw', "%08X" % (s.stw))
        setInfo(info, 'source', s.source)
        setInfo(info, 'ra2000', "%10.4f" % (s.ra2000))
        setInfo(info, 'dec2000', "%10.4f" % (s.dec2000))
        setInfo(info, 'orbit', "%10.4f" % (s.orbit))
        setInfo(info, 'mjd', "%10.3f" % (s.mjd))
        setInfo(info, 'inttime', "%10.3f" % (s.inttime))
        setInfo(info, 'backend', s.backend)
        setInfo(info, 'frontend', s.frontend)
        setInfo(info, 'quality', "%08X" % (s.quality))
        setInfo(info, 'skyfreq', "%10.4f" % (s.skyfreq / 1.0e9))

    def addSpectra(self):
        for i in range(self.lb.index('end')):
            entry = os.path.join(self.dir, self.lb.get(i))
            s = odin.Spectrum(entry)
            if i == 0:
                ave = s.Copy()
            else:
                ave.Accum(s)
            self.showSpectrum(ave)

    def fitTsys(self, cal):
        if cal.frontend == '119':
            Tmin = 500.0
            Tmax = 800.0
        else:
            Tmin = 3000.0
            Tmax = 4000.0

        f = cal.Frequency()
        print cal.freqcal
        x = []
        y = []
        for i in range(cal.channels):
            if cal.data[i] > Tmin and cal.data[i] < Tmax:
                x.append(f[i] - cal.skyfreq / 1.0e9)
                y.append(cal.data[i])
                # print "%d %15.7e %15.7e %10.3f %10.3f" % \
                #       (i, f[i], cal.skyfreq/1.0e9, x[-1],y[-1])
        c = odin.Polyfit(x, y, 3)
        del x
        del y
        print "polynomial coefficients:", c
        for i in range(cal.channels):
            x = f[i] - cal.skyfreq / 1.0e9
            y = c[0] + (c[1] + (c[2] + c[3] * x) * x) * x
            cal.data[i] = y

    def calSpectra(self):
        cal = odin.Spectrum()
        ref = odin.Spectrum()
        tcal = 0.0
        tref = 0.0
        i = 0
        n = self.lb.index('end')
        # for i in range(self.lb.index('end')):
        while i < n:
            entry = os.path.join(self.dir, self.lb.get(i))
            s = odin.Spectrum(entry)
            # print s.type, i, n
            if s.type == "SIG":
                i = i + 1
                pass
            else:
                s.Scale(s.inttime)
                if s.type == "CAL":
                    self.lb.delete(i)
                    n = n - 1
                    if tcal == 0.0:
                        cal = s.Copy()
                        tcal = s.inttime
                    else:
                        cal.Add(s)
                        tcal = tcal + s.inttime
                else:
                    i = i + 1
                    if tref == 0.0:
                        ref = s.Copy()
                        tref = s.inttime
                    else:
                        ref.Add(s)
                        tref = tref + s.inttime

        cal.inttime = tcal
        ref.inttime = tref
        cal.Scale(1.0 / tcal)
        ref.Scale(1.0 / tref)
        print "total cal", cal.inttime
        print "total ref", ref.inttime

        #        self.showSpectrum(cal)
        #        self.master.update()
        #        time.sleep(1)

        #        self.showSpectrum(ref)
        #        self.master.update()
        #        time.sleep(1)

        self.g.element_configure("Two", hide=1)
        self.g.element_configure("One", hide=0)
        cal.Gain(ref)
        self.fitTsys(cal)
        self.showSpectrum(cal)
        self.g.yaxis_configure(min="", max="", title="")
        self.g.xaxis_configure(min="", max="",
                               descending=0,
                               title="frequency [GHz]")
        self.master.update()
        time.sleep(5)

        self.g.yaxis_configure(min=-10, max=300)

        ref = None

        i = 0
        n = self.lb.index('end')
        calibrated = []
        while i < n:
            entry = os.path.join(self.dir, self.lb.get(i))
            s = odin.Spectrum(entry)
            if s.type == "CAL":
                self.lb.delete(i)
                n = n - 1
            else:
                if s.type == "SIG" and ref:
                    new = string.replace(self.lb.get(i), "SIG", "SPE")
                    print new
                    self.lb.insert(i, new)
                    self.lb.delete(i + 1)
                    s.Calibrate(ref, cal)
                    calibrated.append(s)
                    self.showSpectrum(s)
                    self.master.update()
                    self.lb.selection_set(i)
                    self.lb.see(i)
                    i = i + 1
                else:
                    self.lb.delete(i)
                    n = n - 1
                    ref = s.Copy()

        dir = odinfile.selectSavedir(self.master)
        if dir:
            self.dir = dir
            for i in range(self.lb.index('end')):
                entry = os.path.join(self.dir, self.lb.get(i))
                s = calibrated[i]
                print "save scan", entry
                s.Save(entry)

    def showSpectrum(self, s):
        r = s.Copy()
        n = r.channels
        if n > 0:
            self.fillHeader(r)
            bands = 1
            # r.FreqSort()
            if r.backend != "AOS":
                if (r.quality & 0x02000000) == 0:
                    if r.intmode & 0x0100:
                        mode = r.intmode & 0x00ff
                        mode = mode >> 1
                        while mode > 0:
                            if mode & 1:
                                bands = bands + 1
                            mode = mode >> 1
                    else:
                        mode = r.intmode & 0x0007
                        if mode in range(1, 5):
                            bands = 2**(mode - 1)
                        if bands > 1 and r.intmode & 0x10:
                            bands = bands / 2
            x = r.Frequency()
            y = r.data
            for band in range(bands):
                i0 = band * n / bands
                i1 = i0 + n / bands
                curvename = "band" + str(band)
                self.g.element_configure(curvename,
                                         xdata=tuple(x[i0:i1]),
                                         ydata=tuple(y[i0:i1]))
            x = ()
            y = ()
            for band in range(bands, 8):
                curvename = "band" + str(band)
                self.g.element_configure(curvename,
                                         xdata=tuple(x),
                                         ydata=tuple(y))


if __name__ == '__main__':
    root = Tk(className="Odin")
    sv = SpectrumViewer(root)
    sv.master.mainloop()
