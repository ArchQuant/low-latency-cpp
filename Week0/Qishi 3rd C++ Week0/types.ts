
export interface Member {
  name: string;
  background: string;
  techStack: string;
  motivation: string;
}

export interface SyllabusItem {
  week: number;
  title: string;
  description: string;
  details: string[];
  speaker?: string;
}

export interface Resource {
  title: string;
  url?: string;
  description: string;
  level: 'Beginner' | 'Intermediate' | 'Advanced' | 'Video' | 'Reference';
}

export interface STLComponent {
  name: string;
  description: string;
  applications: {
    ai: string;
    crypto: string;
    hft: string;
  };
}

export enum Tab {
  Orientation = 'Orientation',
  Roster = 'Roster',
  Syllabus = 'Syllabus',
  Resources = 'Resources',
  Guidelines = 'Guidelines'
}
