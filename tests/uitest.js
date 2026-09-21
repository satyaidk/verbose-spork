const fs=require('fs'), {JSDOM}=require('jsdom');
const state=JSON.parse(fs.readFileSync('state.json','utf8'));
const html=fs.readFileSync('page.html','utf8');
const posts=[]; const errs=[];
global.fetch=(u,o)=>{ if(u==='/api/state') return Promise.resolve({json:()=>Promise.resolve(state)});
  posts.push({url:u,body:o&&o.body?o.body.toString():''}); return Promise.resolve({text:()=>Promise.resolve('ok')}); };
const dom=new JSDOM(html,{runScripts:'dangerously',pretendToBeVisual:true,
  beforeParse(w){w.fetch=global.fetch;w.confirm=()=>true;w.URLSearchParams=URLSearchParams;w.onerror=m=>errs.push(m);}});
const w=dom.window,d=w.document;
setTimeout(()=>{
  const $=i=>d.getElementById(i); let f=0;
  const t=(n,c,x='')=>{console.log(`${n.padEnd(58)} ${c?'ok':'FAIL'} ${x}`); return c?0:1;};
  f+=t('page renders with no script error',errs.length===0,errs.join(';'));
  f+=t('five tabs present',d.querySelectorAll('nav b').length===5);
  f+=t('mode switch reflects the device',
       ($('m0').className==='on')===(state.mode===0));
  f+=t('16 clips in the grid',d.querySelectorAll('#gifs input').length===16);
  f+=t('six clip selectors populated',
       ['def','intro','tap','dbl','trip','long'].every(k=>$(k).options.length>=16));
  f+=t('triple-tap selector offers None',$('trip').querySelector('option[value="255"]')!==null);
  f+=t('six melody slots loaded',
       ['mIntro','mTap','mDbl','mTrip','mLong','mNotify'].every(k=>$(k).value.length>0));
  f+=t('clock tab shows timezone from device',$('tz').value===state.clk.tz);
  f+=t('notes text loaded',$('ntext').value===state.clk.text);
  f+=t('live panel rendered',d.querySelectorAll('#liveBox div').length===6);
  f+=t('note preview wraps to 21 chars',
       $('notePrev').textContent.split('\n').every(l=>l.length<=21));
  posts.length=0; d.querySelectorAll('nav b')[2].click();
  f+=t('clock tab selectable',$('clk').className==='sel');
  posts.length=0; w.setMode(1);
  f+=t('mode button posts /api/mode with m=1',posts.some(p=>p.url==='/api/mode'&&p.body.includes('m=1')));
  posts.length=0; $('trip').value='5'; $('trip').dispatchEvent(new w.Event('change'));
  f+=t('triple clip change posts trip=5',posts.some(p=>p.url==='/api/gif'&&p.body.includes('trip=5')));
  posts.length=0; $('tz').value='GMT0'; $('tz').dispatchEvent(new w.Event('change'));
  f+=t('timezone change posts /api/clock',posts.some(p=>p.url==='/api/clock'&&p.body.includes('tz=GMT0')));
  posts.length=0; $('ntext').value='hello there'; $('ntext').dispatchEvent(new w.Event('change'));
  f+=t('note text posts to /api/clock',posts.some(p=>p.url==='/api/clock'&&p.body.includes('text=hello')));
  posts.length=0; d.querySelector('.chips[data-for="mTrip"] button').click();
  f+=t('triple-tap preset chip applies and previews',
       posts.some(p=>p.url==='/api/snd')&&posts.some(p=>p.url==='/api/preview'));
  posts.length=0; w.save();
  f+=t('Save settings hits /api/save',posts.some(p=>p.url==='/api/save'));
  // the highlight updates inside the fetch promise, so check on a later tick
  setTimeout(()=>{
    f+=t('mode button highlight follows the response',$('m1').className==='on');
    console.log(f?`\n${f} UI FAILURES`:'\nALL UI CHECKS PASSED'); process.exit(f?1:0);
  },50);
},400);
