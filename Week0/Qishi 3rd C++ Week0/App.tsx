
import React, { useState, useMemo } from 'react';
import { Tab, Member, SyllabusItem, Resource, STLComponent } from './types';
import { MEMBERS, SYLLABUS, RESOURCES, STL_COMPONENTS } from './constants';
import { 
  Users, 
  BookOpen, 
  Calendar, 
  FileText, 
  Youtube, 
  ExternalLink, 
  ChevronRight,
  Clock,
  Video,
  Info,
  BarChart,
  Trophy,
  Target,
  ShieldCheck,
  Star,
  Mic,
  Cpu,
  Layers,
  Zap
} from 'lucide-react';

const App: React.FC = () => {
  const [activeTab, setActiveTab] = useState<Tab>(Tab.Orientation);

  const renderContent = () => {
    switch (activeTab) {
      case Tab.Orientation:
        return <OrientationView />;
      case Tab.Roster:
        return <RosterView />;
      case Tab.Syllabus:
        return <SyllabusView />;
      case Tab.Resources:
        return <ResourcesView />;
      case Tab.Guidelines:
        return <GuidelinesView />;
      default:
        return <OrientationView />;
    }
  };

  return (
    <div className="min-h-screen flex flex-col md:flex-row">
      {/* Sidebar Navigation */}
      <nav className="w-full md:w-64 bg-slate-900 text-slate-300 md:fixed md:h-full z-10">
        <div className="p-6">
          <div className="flex items-center gap-3 mb-8">
            <div className="bg-blue-600 p-2 rounded-lg">
              <span className="text-white font-bold text-xl">Qishi</span>
            </div>
            <div>
              <h1 className="text-white font-bold leading-none">Qishi Club</h1>
              <p className="text-xs text-slate-500 mt-1">C++ Study Group</p>
            </div>
          </div>
          
          <div className="space-y-1">
            <NavItem 
              icon={<Calendar size={18} />} 
              label="Orientation" 
              active={activeTab === Tab.Orientation} 
              onClick={() => setActiveTab(Tab.Orientation)} 
            />
            <NavItem 
              icon={<Users size={18} />} 
              label="Member Profile" 
              active={activeTab === Tab.Roster} 
              onClick={() => setActiveTab(Tab.Roster)} 
            />
            <NavItem 
              icon={<BookOpen size={18} />} 
              label="Syllabus" 
              active={activeTab === Tab.Syllabus} 
              onClick={() => setActiveTab(Tab.Syllabus)} 
            />
            <NavItem 
              icon={<Youtube size={18} />} 
              label="Resources" 
              active={activeTab === Tab.Resources} 
              onClick={() => setActiveTab(Tab.Resources)} 
            />
            <NavItem 
              icon={<FileText size={18} />} 
              label="Guide" 
              active={activeTab === Tab.Guidelines} 
              onClick={() => setActiveTab(Tab.Guidelines)} 
            />
          </div>
        </div>
        
        <div className="absolute bottom-0 w-full p-6 border-t border-slate-800 hidden md:block">
          <p className="text-xs text-slate-500 italic">"Tomorrow's code is only as fast as today's design."</p>
        </div>
      </nav>

      {/* Main Content Area */}
      <main className="flex-1 md:ml-64 p-4 md:p-10 bg-slate-50 overflow-y-auto">
        <div className="max-w-5xl mx-auto">
          {renderContent()}
        </div>
      </main>
    </div>
  );
};

const NavItem: React.FC<{ icon: React.ReactNode; label: string; active: boolean; onClick: () => void }> = ({ icon, label, active, onClick }) => (
  <button 
    onClick={onClick}
    className={`w-full flex items-center gap-3 px-4 py-3 rounded-xl transition-all ${
      active ? 'bg-blue-600 text-white shadow-lg' : 'hover:bg-slate-800 hover:text-white'
    }`}
  >
    {icon}
    <span className="font-medium text-sm">{label}</span>
  </button>
);

const OrientationView: React.FC = () => (
  <div className="space-y-8 animate-in fade-in duration-500">
    <header className="mb-8">
      <div className="inline-block px-3 py-1 rounded-full bg-blue-100 text-blue-700 text-xs font-bold mb-3 uppercase tracking-wider">
        Upcoming Session
      </div>
      <h2 className="text-3xl font-extrabold text-slate-900">Kick-off Orientation</h2>
      <p className="text-slate-500 mt-2">Setting expectations and meeting the squad.</p>
    </header>

    <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
      <div className="bg-white p-6 rounded-2xl shadow-sm border border-slate-100 flex flex-col gap-2">
        <div className="text-blue-600 mb-2"><Calendar size={24} /></div>
        <h3 className="font-bold text-slate-800">Date & Time</h3>
        <p className="text-slate-600 text-sm">Feb 15, Sunday</p>
        <p className="text-slate-600 font-bold">8:00 PM EST</p>
      </div>
      <div className="bg-white p-6 rounded-2xl shadow-sm border border-slate-100 flex flex-col gap-2">
        <div className="text-blue-600 mb-2"><Clock size={24} /></div>
        <h3 className="font-bold text-slate-800">Duration</h3>
        <p className="text-slate-600 text-sm">~ 30 Minutes</p>
        <p className="text-slate-500 italic text-xs mt-1">Status: Chill vibes</p>
      </div>
      <div className="bg-white p-6 rounded-2xl shadow-sm border border-slate-100 flex flex-col gap-2">
        <div className="text-blue-600 mb-2"><Video size={24} /></div>
        <h3 className="font-bold text-slate-800">Platform</h3>
        <p className="text-slate-600 text-sm">Online (Zoom/Meets)</p>
        <p className="text-blue-600 underline text-xs cursor-pointer">Link in WeChat Group</p>
      </div>
    </div>

    <div className="bg-blue-900 text-white rounded-3xl p-8 md:p-12 relative overflow-hidden shadow-2xl">
      <div className="relative z-10">
        <h3 className="text-2xl font-bold mb-6">Agenda</h3>
        <ul className="space-y-4">
          <li className="flex gap-4 items-start">
            <span className="bg-blue-700 h-6 w-6 rounded-full flex items-center justify-center text-xs flex-shrink-0 mt-1">1</span>
            <div>
              <h4 className="font-semibold">Member & TA Introduction</h4>
              <p className="text-blue-300 text-sm">Meet the diverse squad of quant professionals and researchers.</p>
            </div>
          </li>
          <li className="flex gap-4 items-start">
            <span className="bg-blue-700 h-6 w-6 rounded-